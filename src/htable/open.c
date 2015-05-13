#include "htable/open.h"

#include <inttypes.h>

#include "log/log.h"
#include "util/unused.h"

static uint64_t hash(htlp* this, uint64_t key) {
	return sth_hash(&this->hash, key);
}

// TODO: MIN_SIZE, too_sparse, too_dense, pick_capacity partially
// duplicated with htable.c
static const uint64_t MIN_SIZE = 2;

static bool too_sparse(uint64_t pairs, uint64_t capacity) {
	// Keep at least MIN_SIZE slots.
	if (capacity <= MIN_SIZE) {
		return false;
	}
	// At least one quarter full.
	return pairs * 4 < capacity;
}

static bool too_dense(uint64_t pairs, uint64_t capacity) {
	// Keep at least MIN_SIZE slots.
	if (capacity < MIN_SIZE) {
		return true;
	}
	// At most three quarters full.
	return pairs * 4 > capacity * 3;
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

static uint64_t next_index(const htlp* this, uint64_t i) {
	return (i + 1) % this->capacity;
}

static bool slot_occupied(htlp* this, uint64_t slot) {
	return this->keys[slot] != HTLP_EMPTY;
}

static const uint32_t KEYS_WITH_HASH_MAX = (1ULL << 32ULL) - 1;

static bool insert_noresize(htlp* this, uint64_t key, uint64_t value) {
	CHECK(key != HTLP_EMPTY, "trying to insert key HTLP_EMPTY");
	const uint64_t key_hash = hash(this, key);

	if (this->keys_with_hash[key_hash] == KEYS_WITH_HASH_MAX) {
		// TODO: Full rehash?
		log_error("cannot insert - overflow in maximum chain size");
		return false;
	}

	if (htlp_find(this, key, NULL)) {
		log_verbose(1, "%" PRIu64 " already in htlp", key);
		return false;
	}

	for (uint64_t i = 0, index = key_hash, traversed = 0;
			traversed < this->capacity;
			index = next_index(this, index), traversed++) {
		if (slot_occupied(this, index)) {
			if (this->keys[index] == key) {
				return false;
			}
			if (hash(this, this->keys[index]) == key_hash) {
				++i;
			}
		} else {
			this->keys[index] = key;
			this->values[index] = value;
			this->keys_with_hash[key_hash]++;

			this->pair_count++;

			return true;
		}
	}

	log_fatal("htlp is completely full (shouldn't happen)");
}

static UNUSED void dump(htlp* this) {
	for (uint64_t i = 0; i < this->capacity; ++i) {
		if (slot_occupied(this, i)) {
			log_info("[%" PRIu64 "]: %" PRIu64 " => %" PRIu64,
					i, this->keys[i], this->values[i]);
		} else {
			log_info("[%" PRIu64 "]: nil", i);
		}
	}
}

static int8_t resize(htlp* this, uint64_t new_capacity) {
	if (new_capacity < this->pair_count) {
		log_error("cannot fit %" PRIu64 " pairs in %" PRIu64 " slots",
				this->pair_count, new_capacity);
		return 1;
	}

	// Cannot use realloc, because this can both upscale and downscale.
	htlp new_this = {
		// Disabled because Precise.
		// .blocks = aligned_alloc(64, sizeof(block) * new_blocks_size),
		.capacity = new_capacity,
		.pair_count = 0
	};
	CHECK(posix_memalign((void**) &new_this.keys, 64,
			sizeof(uint64_t) * new_capacity) == 0,
			"couldn't allocate aligned memory for keys");
	ASSERT(new_this.keys);  // TODO
	CHECK(posix_memalign((void**) &new_this.values, 64,
			sizeof(uint64_t) * new_capacity) == 0,
			"couldn't allocate aligned memory for values");
	ASSERT(new_this.values);  // TODO
	CHECK(posix_memalign((void**) &new_this.keys_with_hash, 64,
			sizeof(uint32_t) * new_capacity) == 0,
			"couldn't allocate aligned memory for keys_with_hash");
	ASSERT(new_this.keys_with_hash);  // TODO

	// Don't try again with the same hash function, even if we fail now.
	sth_init(&new_this.hash, new_capacity, &this->rand);
	new_this.rand = this->rand;

	// log_info("resizing to %" PRIu64, new_this.capacity);

	for (uint64_t i = 0; i < new_capacity; ++i) {
		new_this.keys[i] = HTLP_EMPTY;
		new_this.keys_with_hash[i] = 0;
	}

	// dump(this);

	for (uint64_t i = 0; i < this->capacity; ++i) {
		if (slot_occupied(this, i)) {
			const uint64_t key = this->keys[i],
					value = this->values[i];
			CHECK(insert_noresize(&new_this, key, value),
					"failed to insert %" PRIu64 "=%" PRIu64,
					key, value);
		}
	}

	htlp_destroy(this);
	*this = new_this;

	return 0;
}

static int8_t resize_to_fit(htlp* this, uint64_t to_fit) {
	uint64_t new_capacity = pick_capacity(this->capacity, to_fit);

	// TODO: make this operation a takeback instead?
	if (new_capacity != this->capacity) {
		// log_info("will resize to %" PRIu64 " to fit %" PRIu64,
		// 		new_capacity, to_fit);
		return resize(this, new_capacity);
	}
	return 0;
}

// key_slot: Required.
// last_slot_with_hash: Optional.
static bool scan(htlp* this, uint64_t key,
		uint64_t* key_slot, uint64_t* last_slot_with_hash) {
	// Special case for empty hash tables.
	if (this->capacity == 0) {
		return false;
	}

	const uint64_t key_hash = hash(this, key);
	uint64_t index = key_hash;
	const uint64_t keys_with_hash = this->keys_with_hash[index];

	bool found = false;
	for (uint64_t i = 0, traversed = 0;
			i < keys_with_hash && traversed < this->capacity;
			index = next_index(this, index), ++traversed) {
		if (!slot_occupied(this, index)) {
			continue;
		}
		const uint64_t current_key = this->keys[index];
		if (current_key == key) {
			found = true;

			// (make GCC happy)
			if (last_slot_with_hash != NULL) {
				*last_slot_with_hash = index;
			}

			*key_slot = index;
			if (last_slot_with_hash == NULL) {
				// We don't care and we're done.
				return true;
			}
		}

		if (hash(this, current_key) == key_hash) {
			// if (i == keys_with_hash - 1 &&
			//		last_slot_with_hash != NULL) {
			if (last_slot_with_hash != NULL) {
				*last_slot_with_hash = index;
			}
			++i;
		}
	}
	return found;
}

bool htlp_delete(htlp* this, uint64_t key) {
	log_verbose(1, "htlp_delete(%" PRIx64 ")", key);
	if (this->pair_count == 0) {
		return false;
	}

	if (resize_to_fit(this, this->pair_count - 1)) {
		log_error("failed to resize to fit one less element");
		return false;
	}

	const uint64_t key_hash = hash(this, key);
	uint64_t to_delete, last;
	if (!scan(this, key, &to_delete, &last)) {
		log_verbose(1, "key %" PRIx64 " not present, cannot delete",
				key);
		return false;
	}
	// Shorten the chain by 1.
	// TODO: Lazy deletes?
	this->keys[to_delete] = this->keys[last];
	this->values[to_delete] = this->values[last];
	this->keys[last] = HTLP_EMPTY;
	this->keys_with_hash[key_hash]--;

	this->pair_count--;
	return true;
}

bool htlp_find(htlp* this, uint64_t key, uint64_t *value) {
	uint64_t found_at;
	if (scan(this, key, &found_at, NULL)) {
		log_verbose(1, "htlp_find(%" PRIu64 "): found %" PRIu64,
				key, this->values[found_at]);
		if (value) {
			*value = this->values[found_at];
		}
		return true;
	} else {
		log_verbose(1, "find(%" PRIu64 "): not found", key);
		return false;
	}
}

bool htlp_insert(htlp* this, uint64_t key, uint64_t value) {
	if (resize_to_fit(this, this->pair_count + 1)) {
		log_error("failed to resize to fit one more element");
		return false;
	}
	return insert_noresize(this, key, value);
}

void htlp_init(htlp* this, rand_generator rand) {
	*this = (htlp) {
		.capacity = 0,
		.pair_count = 0,

		.keys = NULL,
		.values = NULL,
		.keys_with_hash = NULL,

		.rand = rand,
	};
}

void htlp_destroy(htlp* this) {
	if (this->keys) {
		free(this->keys);
		this->keys = NULL;
	}
	if (this->values) {
		free(this->values);
		this->values = NULL;
	}
	if (this->keys_with_hash) {
		free(this->keys_with_hash);
		this->keys_with_hash = NULL;
	}
}

