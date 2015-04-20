#include "htable/htable.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "htable/hash.h"
#include "log/log.h"
#include "util/average.h"

static const uint64_t MIN_SIZE = 2;

/*
static void check_invariants(htable* this) {
	uint64_t total = 0;
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		for (int subindex1 = 0; subindex1 < 3; subindex1++) {
			if (this->blocks[i].occupied[subindex1]) total++;
		}

		uint64_t count = 0;
		for (uint64_t j = 0; j < this->blocks_size; j++) {
			for (int subindex = 0; subindex < 3; subindex++) {
				if (this->blocks[j].occupied[subindex] &&
						htable_hash(this, this->blocks[j].keys[subindex]) == i) {
					count++;
				}
			}
		}
		if (count != this->blocks[i].keys_with_hash) {
			log_fatal("bucket %" PRIx64 " claims to have %" PRIu32 " pairs, but actually has %" PRIx64 "!",
				i,
				this->blocks[i].keys_with_hash,
				count);
		}
	}
	CHECK(this->pair_count == total);
	log_info("check_invariants OK");
}
*/

static bool too_sparse(uint64_t pairs, uint64_t blocks) {
	// Keep at least MIN_SIZE blocks.
	if (blocks <= MIN_SIZE) return false;

	uint64_t buckets = blocks * 3;

	// At least one quarter full.
	return pairs * 4 < buckets;
}

static bool too_dense(uint64_t pairs, uint64_t blocks) {
	// Keep at least MIN_SIZE blocks.
	if (blocks < MIN_SIZE) return true;

	uint64_t buckets = blocks * 3;

	// At most three quarters full.
	return pairs * 4 > buckets * 3;
}

static uint64_t next_bigger_size(uint64_t x) {
	if (x < MIN_SIZE) return MIN_SIZE;

	// Next power of 2
	uint64_t i = 1;
	while (i <= x) i *= 2;
	return i;
}

static uint64_t next_smaller_size(uint64_t x) {
	// Previous power of 2
	uint64_t i = 1;
	while (i < x) i *= 2;
	return i / 2;
}

static int8_t foreach(htable* this,
		int8_t (*iterate)(void*, uint64_t key, uint64_t value),
		void* opaque) {
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		const htable_block* current_block = &this->blocks[i];

		for (uint8_t slot = 0; slot < 3; slot++) {
			if (current_block->occupied[slot]) {
				const uint64_t key = current_block->keys[slot],
						value = current_block->values[slot];
				if (iterate(opaque, key, value)) {
					log_error("foreach iteration failed on %ld=%ld",
							key, value);
					return 1;
				}
			}
		}
	}
	return 0;
}

static int8_t insert_internal_wrap(void* this, uint64_t key, uint64_t value) {
	return htable_insert_noresize(this, key, value);
}

static int8_t resize(htable* this, uint64_t new_blocks_size) {
	if (new_blocks_size * 3 < this->pair_count) {
		log_error("cannot resize: %" PRIu64 " blocks don't fit %" PRIu64 " pairs",
			new_blocks_size, this->pair_count);
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
		log_error("cannot allocate memory for %ld blocks", new_blocks_size);
		goto err_1;
	}

	memset(new_this.blocks, 0, sizeof(htable_block) * new_blocks_size);

	if (foreach(this, insert_internal_wrap, &new_this)) {
		log_error("iteration failed");
		goto err_2;
	}

	free(this->blocks);
	this->blocks = new_this.blocks;
	this->blocks_size = new_this.blocks_size;

	return 0;

err_2:
	free(new_this.blocks);
err_1:
	return 1;
}

static int8_t resize_to_fit(htable* this, uint64_t to_fit) {
	uint64_t new_blocks_size = this->blocks_size;

	while (too_sparse(to_fit, new_blocks_size)) {
		new_blocks_size = next_smaller_size(new_blocks_size);
	}

	while (too_dense(to_fit, new_blocks_size)) {
		new_blocks_size = next_bigger_size(new_blocks_size);
	}

	assert(!too_sparse(to_fit, new_blocks_size) &&
			!too_dense(to_fit, new_blocks_size));

	// TODO: make this operation a takeback instead?
	if (new_blocks_size != this->blocks_size) {
		log_info("will resize to %" PRIu64 " to fit %" PRIu64,
				new_blocks_size, to_fit);
		return resize(this, new_blocks_size);
	}

	return 0;
}

const uint32_t HTABLE_KEYS_WITH_HASH_MAX = (1LL << 32LL) - 1;

typedef struct {
	htable_block* block;
	uint8_t slot;
} slot_pointer;

static uint64_t next_index(const htable* this, uint64_t i) {
	return (i + 1) % this->blocks_size;
}

static bool scan(htable* this, uint64_t key,
		slot_pointer* key_slot, slot_pointer* last_slot_with_hash) {
	// Special case for empty hash tables.
	if (this->blocks_size == 0) {
		return false;
	}

	const uint64_t key_hash = htable_hash(this, key);
	uint64_t index = key_hash;
	const uint64_t keys_with_hash = this->blocks[index].keys_with_hash;
	htable_block current_block;

	bool found = false;
	for (uint64_t i = 0; i < keys_with_hash;
			index = next_index(this, index)) {
		// Make a local copy
		current_block = this->blocks[index];

		for (uint8_t slot = 0; slot < 3; slot++) {
			const uint64_t current_key = current_block.keys[slot];

			if (current_block.occupied[slot]) {
				if (current_key == key) {
					found = true;

					if (key_slot != NULL) {
						key_slot->block = &this->blocks[index];
						key_slot->slot = slot;
					}

					if (last_slot_with_hash == NULL) {
						// We don't care and we're done.
						return true;
					}
				}

				if (htable_hash(this, current_key) == key_hash) {
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

int8_t htable_delete(htable* this, uint64_t key) {
	log_verbose(1, "htable_delete(%" PRIx64 ")", key);

	if (this->pair_count == 0) {
		// Empty hash table has no elements.
		return 1;
	}

	if (resize_to_fit(this, this->pair_count - 1)) {
		log_error("failed to resize to fit one less element");
		return 1;
	}

	const uint64_t key_hash = htable_hash(this, key);

	slot_pointer to_delete, last;

	if (!scan(this, key, &to_delete, &last)) {
		log_error("key %" PRIx64 " not present, cannot delete", key);
		return 1;
	}
	// Shorten the chain by 1.
	last.block->occupied[last.slot] = false;
	to_delete.block->keys[to_delete.slot] = last.block->keys[last.slot];
	to_delete.block->values[to_delete.slot] = last.block->values[last.slot];

	this->blocks[key_hash].keys_with_hash--;
	this->pair_count--;

	// check_invariants(this);

	return 0;
}

void htable_find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	htable* this = _this;

	slot_pointer found_at;
	if (scan(this, key, &found_at, NULL)) {
		log_verbose(1, "htable_find(%" PRIu64 "): found %" PRIu64,
				key, found_at.block->values[found_at.slot]);
		*found = true;
		if (value) {
			*value = found_at.block->values[found_at.slot];
		}
	} else {
		log_verbose(1, "find(%" PRIu64 "): not found", key);
		*found = false;
	}
}

static void dump_block(htable* this, uint64_t index, htable_block* block) {
	char buffer[256], buffer2[256];
	snprintf(buffer, sizeof(buffer),
			"[%04" PRIx64 "] keys_with_hash=%" PRIu32,
			index, block->keys_with_hash);

	for (int i = 0; i < 3; i++) {
		if (block->occupied[i]) {
			strncpy(buffer2, buffer, sizeof(buffer2) - 1);
			snprintf(buffer, sizeof(buffer),
					"%s [%016" PRIx64 "(%04" PRIx64 ")=%016" PRIx64 "]",
					buffer2,
					block->keys[i],
					htable_hash(this, block->keys[i]),
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

static void calculate_distances(htable* this, int distances[100]) {
	memset(distances, 0, sizeof(int) * 100);
	// TODO: optimize?
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		for (int8_t slot = 0; slot < 3; slot++) {
			if (!this->blocks[i].occupied[slot]) continue;

			const uint64_t should_be_at =
				htable_hash(this, this->blocks[i].keys[slot]);

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

void htable_dump_blocks(htable* this) {
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		dump_block(this, i, &this->blocks[i]);
	}
}

static void calculate_bucket_sizes(htable* this, int bucket_sizes[100]) {
	memset(bucket_sizes, 0, sizeof(int) * 100);
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		uint64_t bucket_size = this->blocks[i].keys_with_hash;
		assert(bucket_size < 100);
		bucket_sizes[bucket_size]++;
	}
}

void htable_dump_stats(htable* this) {
	log_plain("htable blocks:%ld pair_count:%ld",
			this->blocks_size, this->pair_count);
	if (this->blocks_size > 0) {
		int distances[100] = { 0 };
		calculate_distances(this, distances);
		log_plain("average distance: %.2lf", histogram_average_i(distances, 100));
		for (int i = 0; i < 100; i++) {
			if (distances[i] > 0) {
				log_plain("distance %3d: %8d groups", i, distances[i]);
			}
		}

		int bucket_sizes[100] = { 0 };
		calculate_bucket_sizes(this, bucket_sizes);

		for (int i = 0; i < 100; i++) {
			if (bucket_sizes[i] > 0) {
				log_plain("%d same-hash groups of size %d",
						bucket_sizes[i], i);
			}
		}
		log_plain("average bucket size: %.2lf",
				histogram_average_i(bucket_sizes, 100));
	}
}

int8_t htable_insert(htable* this, uint64_t key, uint64_t value) {
	if (resize_to_fit(this, this->pair_count + 1)) {
		log_error("failed to resize to fit one more element");
		return 1;
	}
	return htable_insert_noresize(this, key, value);
}

int8_t htable_insert_noresize(htable* this, uint64_t key, uint64_t value) {
	const uint64_t key_hash = htable_hash(this, key);
	htable_block* const home_block = &this->blocks[key_hash];

	if (home_block->keys_with_hash == HTABLE_KEYS_WITH_HASH_MAX) {
		log_error("cannot insert - overflow in maximum bucket size");
		return 1;
	}

	for (uint64_t i = 0, index = key_hash, traversed = 0;
			traversed < this->blocks_size;
			index = next_index(this, index), traversed++) {
		htable_block* current_block = &this->blocks[index];

		for (int8_t slot = 0; slot < 3; slot++) {
			if (current_block->occupied[slot]) {
				if (current_block->keys[slot] == key) {
					log_verbose(1, "duplicate in bucket %" PRIu64 " "
						"when inserting %" PRIu64 "=%" PRIu64,
						index, key, value);
					return 1;
				}

				if (htable_hash(this, current_block->keys[slot]) == key_hash) {
					i++;
				}
			} else {
				current_block->occupied[slot] = true;
				current_block->keys[slot] = key;
				current_block->values[slot] = value;

				home_block->keys_with_hash++;

				this->pair_count++;

				return 0;
			}
		}
	}

	// Went over all blocks...
	log_error("htable is completely full (shouldn't happen)");
	return 1;
}
