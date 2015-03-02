#include "dict/hashtable.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define NO_LOG_INFO

#include "log/log.h"

#include "hashtable/private/data.h"
#include "hashtable/private/dump.h"
#include "hashtable/private/hash.h"
#include "hashtable/private/traversal.h"
#include "hashtable/private/resizing.h"
#include "hashtable/private/insertion.h"
#include "hashtable/private/find.h"
#include "hashtable/private/delete.h"

/*
static void check_invariants(struct hashtable_data* this) {
	uint64_t total = 0;
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		for (int subindex1 = 0; subindex1 < 3; subindex1++) {
			if (this->blocks[i].occupied[subindex1]) total++;
		}

		uint64_t count = 0;
		for (uint64_t j = 0; j < this->blocks_size; j++) {
			for (int subindex = 0; subindex < 3; subindex++) {
				if (this->blocks[j].occupied[subindex] &&
						hashtable_hash_of(this, this->blocks[j].keys[subindex]) == i) {
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

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	struct hashtable_data* this = malloc(sizeof(struct hashtable_data));
	if (!this) {
		log_error("cannot allocate new hashtable");
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

	log_info("insert(%" PRIx64 ", %" PRIx64 ")", key, value);

	if (hashtable_resize_to_fit(this, this->pair_count + 1)) {
		log_error("failed to resize to fit one more element");
		return 1;
	}

	return hashtable_insert_internal(this, key, value);
}

static int8_t delete(void* _this, uint64_t key) {
	struct hashtable_data* this = _this;
	return hashtable_delete(this, key);
}

const dict_api dict_hashtable = {
	.init = init,
	.destroy = destroy,

	.find = hashtable_find,
	.insert = insert,
	.delete = delete,

	.dump = hashtable_dump,

	.name = "dict_hashtable"
};