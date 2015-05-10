#include "htable/htable.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "htable/hash.h"
#include "log/log.h"
#include "util/average.h"

static uint64_t hash(htable* this, uint64_t key) {
	if (this->hash_fn_override) {
		return this->hash_fn_override(this->hash_fn_override_opaque,
				key);
	} else {
		return fnv_hash(key, this->blocks_size);
	}
}

static const uint64_t MIN_SIZE = 2;

static bool too_sparse(uint64_t pairs, uint64_t blocks) {
	// Keep at least MIN_SIZE blocks.
	if (blocks <= MIN_SIZE) {
		return false;
	}
	const uint64_t slots = blocks * SLOTS_PER_BLOCK;
	// At least one quarter full.
	return pairs * 4 < slots;
}

static bool too_dense(uint64_t pairs, uint64_t blocks) {
	// Keep at least MIN_SIZE blocks.
	if (blocks < MIN_SIZE) {
		return true;
	}
	const uint64_t slots = blocks * SLOTS_PER_BLOCK;
	// At most three quarters full.
	return pairs * 4 > slots * 3;
}

static uint64_t pick_capacity(uint64_t current_size, uint64_t pairs) {
	uint64_t new_size = current_size;

	while (too_sparse(pairs, new_size)) {
		new_size /= 2;
	}

	while (too_dense(pairs, new_size)) {
		new_size *= 2;
		if (new_size < MIN_SIZE) {
			new_size = MIN_SIZE;
		}
	}

	assert(!too_sparse(pairs, new_size) && !too_dense(pairs, new_size));
	return new_size;
}

static uint64_t next_index(const htable* this, uint64_t i) {
	return (i + 1) % this->blocks_size;
}

static const uint32_t KEYS_WITH_HASH_MAX = (1LL << 32LL) - 1;

static bool insert_noresize(htable* this, uint64_t key, uint64_t value) {
	const uint64_t key_hash = hash(this, key);
	htable_block* const home_block = &this->blocks[key_hash];

	if (home_block->keys_with_hash == KEYS_WITH_HASH_MAX) {
		log_error("cannot insert - overflow in maximum block size");
		return false;
	}

	for (uint64_t i = 0, index = key_hash, traversed = 0;
			traversed < this->blocks_size;
			index = next_index(this, index), traversed++) {
		htable_block* current_block = &this->blocks[index];

		for (int8_t slot = 0; slot < SLOTS_PER_BLOCK; slot++) {
			if (current_block->occupied[slot]) {
				if (current_block->keys[slot] == key) {
					log_verbose(1, "duplicate in block %" PRIu64 " "
						"when inserting %" PRIu64 "=%" PRIu64,
						index, key, value);
					return false;
				}

				if (hash(this, current_block->keys[slot]) ==
						key_hash) {
					i++;
				}
			} else {
				current_block->occupied[slot] = true;
				current_block->keys[slot] = key;
				current_block->values[slot] = value;

				home_block->keys_with_hash++;

				this->pair_count++;
				return true;
			}
		}
	}

	// Went over all blocks...
	log_fatal("htable is completely full (shouldn't happen)");
}

static int8_t resize(htable* this, uint64_t new_blocks_size) {
	if (new_blocks_size * SLOTS_PER_BLOCK < this->pair_count) {
		log_error("cannot fit %" PRIu64 " pairs in %" PRIu64 " blocks",
				this->pair_count, new_blocks_size);
		goto err_1;
	}
	// TODO: try new hash function?

	// Cannot use realloc, because this can both upscale and downscale.
	htable new_this = {
		// Disabled because Precise.
		// .blocks = aligned_alloc(64, sizeof(block) * new_blocks_size),
		.blocks_size = new_blocks_size,
		.pair_count = 0
	};
	CHECK(posix_memalign((void**) &new_this.blocks, 64,
			sizeof(htable_block) * new_blocks_size) == 0,
			"couldn't allocate aligned memory for htable");

	log_info("resizing to %" PRIu64, new_this.blocks_size);

	if (!new_this.blocks) {
		log_error("cannot allocate %ld blocks", new_blocks_size);
		goto err_1;
	}

	memset(new_this.blocks, 0, sizeof(htable_block) * new_blocks_size);

	for (uint64_t i = 0; i < this->blocks_size; i++) {
		const htable_block* current_block = &this->blocks[i];

		for (uint8_t slot = 0; slot < SLOTS_PER_BLOCK; slot++) {
			if (current_block->occupied[slot]) {
				const uint64_t key = current_block->keys[slot],
						value = current_block->values[slot];
				if (!insert_noresize(&new_this, key, value)) {
					log_error("failed to insert "
							"%" PRIu64 "=%" PRIu64,
							key, value);
					goto err_2;
				}
			}
		}
	}

	htable_destroy(this);
	*this = new_this;

	return 0;

err_2:
	htable_destroy(&new_this);
err_1:
	return 1;
}

static int8_t resize_to_fit(htable* this, uint64_t to_fit) {
	uint64_t new_blocks_size = pick_capacity(this->blocks_size, to_fit);

	// TODO: make this operation a takeback instead?
	if (new_blocks_size != this->blocks_size) {
		log_info("will resize to %" PRIu64 " to fit %" PRIu64,
				new_blocks_size, to_fit);
		return resize(this, new_blocks_size);
	}

	return 0;
}

typedef struct {
	htable_block* block;
	uint8_t slot;
} slot_pointer;

// key_slot: Required.
// last_slot_with_hash: Optional.
static bool scan(htable* this, uint64_t key,
		slot_pointer* key_slot, slot_pointer* last_slot_with_hash) {
	// Special case for empty hash tables.
	if (this->blocks_size == 0) {
		return false;
	}

	const uint64_t key_hash = hash(this, key);
	uint64_t index = key_hash;
	const uint64_t keys_with_hash = this->blocks[index].keys_with_hash;
	htable_block current_block;

	bool found = false;
	for (uint64_t i = 0; i < keys_with_hash;
			index = next_index(this, index)) {
		// Make a local copy
		current_block = this->blocks[index];

		for (uint8_t slot = 0; slot < SLOTS_PER_BLOCK; slot++) {
			const uint64_t current_key = current_block.keys[slot];

			if (current_block.occupied[slot]) {
				if (current_key == key) {
					found = true;

					key_slot->block = &this->blocks[index];
					key_slot->slot = slot;

					if (last_slot_with_hash == NULL) {
						// We don't care and we're done.
						return true;
					}
				}

				if (hash(this, current_key) == key_hash) {
					if (i == keys_with_hash - 1 &&
							last_slot_with_hash != NULL) {
						last_slot_with_hash->block = &this->blocks[index];
						last_slot_with_hash->slot = slot;
					}
					i++;
				}
			}
		}
		// TODO: guard against infinite loop?
	}
	return found;
}

bool htable_delete(htable* this, uint64_t key) {
	log_verbose(1, "htable_delete(%" PRIx64 ")", key);
	if (this->pair_count == 0) {
		return false;
	}

	if (resize_to_fit(this, this->pair_count - 1)) {
		log_error("failed to resize to fit one less element");
		return false;
	}

	const uint64_t key_hash = hash(this, key);

	slot_pointer to_delete, last;

	if (!scan(this, key, &to_delete, &last)) {
		log_error("key %" PRIx64 " not present, cannot delete", key);
		return false;
	}
	// Shorten the chain by 1.
	last.block->occupied[last.slot] = false;
	to_delete.block->keys[to_delete.slot] = last.block->keys[last.slot];
	to_delete.block->values[to_delete.slot] = last.block->values[last.slot];

	this->blocks[key_hash].keys_with_hash--;
	this->pair_count--;
	return true;
}

bool htable_find(htable* this, uint64_t key, uint64_t *value) {
	slot_pointer found_at;
	if (scan(this, key, &found_at, NULL)) {
		log_verbose(1, "htable_find(%" PRIu64 "): found %" PRIu64,
				key, found_at.block->values[found_at.slot]);
		if (value) {
			*value = found_at.block->values[found_at.slot];
		}
		return true;
	} else {
		log_verbose(1, "find(%" PRIu64 "): not found", key);
		return false;
	}
}

bool htable_insert(htable* this, uint64_t key, uint64_t value) {
	if (resize_to_fit(this, this->pair_count + 1)) {
		log_error("failed to resize to fit one more element");
		return false;
	}
	return insert_noresize(this, key, value);
}

void htable_init(htable* this) {
	*this = (htable) {
		.blocks = NULL,
		.blocks_size = 0,
		.pair_count = 0
	};
}

void htable_destroy(htable* this) {
	if (this->blocks) {
		free(this->blocks);
	}
}

// Debugging.

static void dump_block(htable* this, uint64_t index, htable_block* block) {
	char buffer[256], buffer2[256];
	snprintf(buffer, sizeof(buffer),
			"[%04" PRIx64 "] keys_with_hash=%" PRIu32,
			index, block->keys_with_hash);

	for (int i = 0; i < SLOTS_PER_BLOCK; i++) {
		if (block->occupied[i]) {
			strncpy(buffer2, buffer, sizeof(buffer2) - 1);
			snprintf(buffer, sizeof(buffer),
					"%s [%016" PRIx64 "(%04" PRIx64 ")=%016" PRIx64 "]",
					buffer2,
					block->keys[i],
					hash(this, block->keys[i]),
					block->values[i]);
		} else {
			strncpy(buffer2, buffer, sizeof(buffer2) - 1);
			snprintf(buffer, sizeof(buffer),
					"%s [                (    )=                ]",
					buffer2);
		}
	}

	log_plain("%s", buffer);
}

void htable_dump_blocks(htable* this) {
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		dump_block(this, i, &this->blocks[i]);
	}
}

static void calculate_distances(htable* this, int distances[100]) {
	memset(distances, 0, sizeof(int) * 100);
	// TODO: optimize?
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		for (int8_t slot = 0; slot < SLOTS_PER_BLOCK; slot++) {
			if (!this->blocks[i].occupied[slot]) continue;

			const uint64_t should_be_at =
					hash(this, this->blocks[i].keys[slot]);

			uint64_t distance;
			if (should_be_at <= i) {
				distance = i - should_be_at;
			} else {
				distance = (this->blocks_size - should_be_at) + i;
			}
			distances[distance]++;
		}
	}
}

static void calculate_block_sizes(htable* this, int block_sizes[100]) {
	memset(block_sizes, 0, sizeof(int) * 100);
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		const uint64_t block_size = this->blocks[i].keys_with_hash;
		assert(block_size < 100);
		++block_sizes[block_size];
	}
}

void htable_dump_stats(htable* this) {
	log_plain("htable blocks:%ld pair_count:%ld",
			this->blocks_size, this->pair_count);
	if (this->blocks_size > 0) {
		int distances[100] = { 0 };
		calculate_distances(this, distances);
		log_plain("average distance: %.2lf",
				histogram_average_i(distances, 100));
		for (int i = 0; i < 100; i++) {
			if (distances[i] > 0) {
				log_plain("distance %3d: %8d groups",
						i, distances[i]);
			}
		}

		int block_sizes[100] = { 0 };
		calculate_block_sizes(this, block_sizes);

		for (int i = 0; i < 100; i++) {
			if (block_sizes[i] > 0) {
				log_plain("%d same-hash groups of size %d",
						block_sizes[i], i);
			}
		}
		log_plain("average block size: %.2lf",
				histogram_average_i(block_sizes, 100));
	}
}
