#include "hash_hashtable.h"
#include "private/data.h"
#include "private/dump.h"
#include "private/hash.h"
#include "private/traversal.h"
#include "private/resizing.h"
#include "private/insertion.h"
#include "private/find.h"

#define NO_LOG_INFO

#include "../log/log.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

/*
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
*/

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	struct hashtable_data* this = malloc(sizeof(struct hashtable_data));
	if (!this) {
		log_error("cannot allocate new hash_hashtable");
		return 1;
	}

	*this = (struct hashtable_data) {
		.blocks = NULL,
		.blocks_size = 0,
		.pair_count = 0
	};
	*_this = this;
	return 0;
}

static void destroy(void** _this) {
	if (_this) {
		struct hashtable_data* this = *_this;
		if (this) {
			free(this->blocks);
		}
		free(this);
		*_this = NULL;
	}
}

const uint32_t HASHTABLE_KEYS_WITH_HASH_MAX = (1LL << 32LL) - 1;

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	struct hashtable_data* this = _this;

	log_info("insert(%" PRIu64 ", %" PRIu64 ")", key, value);

	if (hashtable_resize_to_fit(this, this->pair_count + 1)) {
		log_error("failed to resize to fit one more element");
		return 1;
	}

	return hashtable_insert_internal(this, key, value);
}

static int8_t delete(void* _this, uint64_t key) {
	struct hashtable_data* this = _this;

	log_info("delete(%" PRIu64 ")", key);

	if (hashtable_resize_to_fit(this, this->pair_count - 1)) {
		log_error("failed to resize to fit one less element");
		return 1;
	}

	uint64_t key_hash = hashtable_hash_of(this, key);

	uint64_t index = key_hash;
	uint64_t keys_with_hash = this->blocks[key_hash].keys_with_hash;

	for (uint64_t i = 0; i < keys_with_hash; index = hashtable_next_index(this, index)) {
		struct hashtable_block* block = &this->blocks[index];

		for (int subindex = 0; subindex < 3; subindex++) {
			if (block->occupied[subindex]) {
				if (block->keys[subindex] == key) {
					block->occupied[subindex] = false;
					this->blocks[key_hash].keys_with_hash--;
					this->pair_count--;

					// check_invariants(this);

					return 0;
				}

				if (hashtable_hash_of(this, block->keys[subindex])) {
					i++;
				}
			}
		}
	}

	log_error("key %" PRIu64 " not present, cannot delete", key);
	return 1;
}

const hash_api hash_hashtable = {
	.init = init,
	.destroy = destroy,

	.find = hashtable_find,
	.insert = insert,
	.delete = delete,

	.dump = hashtable_dump,

	.name = "hash_hashtable"
};
