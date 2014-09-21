#include "insertion.h"
#include "hash.h"
#include "traversal.h"

#include <inttypes.h>

#include "../../log/log.h"

int8_t hashtable_insert_internal(struct hashtable_data* this, uint64_t key, uint64_t value) {
	uint64_t key_hash = hashtable_hash_of(this, key);
	struct hashtable_block* home_block = &this->blocks[key_hash];

	if (home_block->keys_with_hash == HASHTABLE_KEYS_WITH_HASH_MAX) {
		log_error("cannot insert - overflow in maximum bucket size");
		return 1;
	}

	bool free_index_found = false;

	for (uint64_t i = 0, index = key_hash;
		i < home_block->keys_with_hash || !free_index_found;
		index = hashtable_next_index(this, index)
	) {
		struct hashtable_block* block = &this->blocks[index];

		for (int subindex = 0; subindex < 3; subindex++) {
			if (block->occupied[subindex]) {
				if (block->keys[subindex] == key) {
					log_error("duplicate in bucket %" PRIu64 " "
						"when inserting %" PRIu64 "=%" PRIu64,
						index, key, value);
					return 1;
				}

				if (hashtable_hash_of(this, block->keys[subindex]) == key_hash) {
					i++;
				}
			} else {
				block->occupied[subindex] = true;
				block->keys[subindex] = key;
				block->values[subindex] = value;

				home_block->keys_with_hash++;

				this->pair_count++;

				return 0;
			}
		}
	}

	log_error("?!");
	return 1;
}
