#include "hash_hashtable.h"
#include "private/data.h"
#include "private/dump.h"
#include "private/hash.h"
#include "private/traversal.h"
#include "private/resizing.h"
#include "private/insertion.h"

#define NO_LOG_INFO

#include "../log/log.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

static const uint64_t MIN_SIZE = 4; // 8;

static void check_invariants(struct hashtable_data* this) {
	uint64_t total = 0;
	for (uint64_t i = 0; i < this->table_size; i++) {
		if (this->table[i].occupied) total++;

		uint64_t count = 0;
		for (uint64_t j = 0; j < this->table_size; j++) {
			if (this->table[j].occupied && hash_of(this, this->table[j].key) == i) {
				count++;
			}
		}
		if (count != this->table[i].keys_with_hash) {
			log_fatal("bucket %" PRIu64 " claims to have %" PRIu32 " pairs, but actually has %" PRIu64 "!",
				i,
				this->table[i].keys_with_hash,
				count);
		}
	}
	CHECK(this->pair_count == total);
	log_info("check_invariants OK");
}

static bool too_sparse(uint64_t pairs, uint64_t buckets) {
	// Keep at least MIN_SIZE buckets.
	if (buckets <= MIN_SIZE) return false;

	// At least one quarter full.
	return pairs * 4 < buckets;
}

static bool too_dense(uint64_t pairs, uint64_t buckets) {
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

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	struct hashtable_data* this = malloc(sizeof(struct hashtable_data));
	if (!this) {
		log_error("cannot allocate new hash_hashtable");
		goto err_1;
	}

	*this = (struct hashtable_data) {
		.table = malloc(sizeof(struct hashtable_bucket) * MIN_SIZE),
		.table_size = MIN_SIZE,
		.pair_count = 0
	};

	if (!this->table) {
		log_error("cannot allocate hash table");
		goto err_2;
	}
	memset(this->table, 0, sizeof(struct hashtable_bucket) * MIN_SIZE);

	*_this = this;

	return 0;

err_2:
	free(this);
err_1:
	return 1;
}

static void destroy(void** _this) {
	if (_this) {
		struct hashtable_data* this = *_this;
		if (this) {
			free(this->table);
		}
		free(this);
		*_this = NULL;
	}
}

static int8_t find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	struct hashtable_data* this = _this;

	log_info("find(%" PRIu64 "(h=%" PRIu64 "))", key, hash_of(this, key));

	uint64_t key_hash = hash_of(this, key);

	uint64_t index = key_hash;
	uint64_t keys_with_hash = this->table[index].keys_with_hash;

	for (uint64_t i = 0; i < keys_with_hash; index = hashtable_next_index(this, index)) {
		if (this->table[index].occupied) {
			if (this->table[index].key == key) {
				*found = true;
				if (value) *value = this->table[index].value;
				log_info("find(%" PRIu64 "): found, value=%" PRIu64, key, this->table[index].value);
				return 0;
			}

			if (hash_of(this, this->table[index].key) == key_hash) {
				i++;
			}
		}
		// TODO: guard against infinite loop?
	}

	log_info("find(%" PRIu64 "): not found", key);
	*found = false;
	return 0;
}

const uint32_t HASHTABLE_KEYS_WITH_HASH_MAX = (1LL << 32LL) - 1;

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	struct hashtable_data* this = _this;

	log_info("insert(%" PRIu64 ", %" PRIu64 ")", key, value);

	uint64_t new_size = this->table_size;
	while (too_dense(this->pair_count + 1, new_size)) {
		new_size = next_bigger_size(new_size);
	}
	if (new_size != this->table_size) {
		if (hashtable_resize(this, next_bigger_size(this->table_size))) {
			log_error("failed to resize when inserting %ld=%ld", key, value);
			return 1;
		}
	}

	return hashtable_insert_internal(this, key, value);
}

static int8_t delete(void* _this, uint64_t key) {
	struct hashtable_data* this = _this;

	log_info("delete(%" PRIu64 ")", key);

	uint64_t new_size = this->table_size;
	while (too_sparse(this->pair_count - 1, new_size)) {
		new_size = next_smaller_size(new_size);
	}
	if (new_size != this->table_size) {
		if (hashtable_resize(this, next_smaller_size(new_size))) {
			log_error("failed to resize while deleting key %ld", key);
			return 1;
		}
	}

	uint64_t key_hash = hash_of(this, key);

	uint64_t index = key_hash;
	uint64_t keys_with_hash = this->table[key_hash].keys_with_hash;

	for (uint64_t i = 0; i < keys_with_hash; index = hashtable_next_index(this, index)) {
		if (this->table[index].occupied) {
			if (this->table[index].key == key) {
				this->table[index].occupied = false;
				this->table[key_hash].keys_with_hash--;
				this->pair_count--;

				// check_invariants(this);

				return 0;
			}

			if (hash_of(this, this->table[index].key) == key_hash) {
				i++;
			}
		}
	}

	log_error("key %" PRIu64 " not present, cannot delete", key);
	return 1;
}

const hash_api hash_hashtable = {
	.init = init,
	.destroy = destroy,

	.find = find,
	.insert = insert,
	.delete = delete,

	.dump = hashtable_dump,

	.name = "hash_hashtable"
};
