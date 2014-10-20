#include "insertion.h"
#include "hash.h"
#include "traversal.h"
#include "helper.h"

#include <inttypes.h>

#include "../../log/log.h"

int8_t hashtable_insert_internal(hashtable* this, uint64_t key, uint64_t value) {
	uint64_t key_hash = hashtable_hash_of(this, key);
	block* home_block = &this->blocks[key_hash];

	if (home_block->keys_with_hash == HASHTABLE_KEYS_WITH_HASH_MAX) {
		log_error("cannot insert - overflow in maximum bucket size");
		return 1;
	}

	bool free_index_found = false;

	for (uint64_t i = 0, index = key_hash;
		i < home_block->keys_with_hash || !free_index_found;
		index = hashtable_next_index(this, index)
	) {
		block* current_block = &this->blocks[index];

		for (int8_t slot = 0; slot < 3; slot++) {
			if (current_block->occupied[slot]) {
				if (current_block->keys[slot] == key) {
					log_error("duplicate in bucket %" PRIu64 " "
						"when inserting %" PRIu64 "=%" PRIu64,
						index, key, value);
					return 1;
				}

				if (hashtable_hash_of(this, current_block->keys[slot]) == key_hash) {
					i++;
				}
			} else {
				free_index_found = true;

				current_block->occupied[slot] = true;
				current_block->keys[slot] = key;
				current_block->values[slot] = value;

				home_block->keys_with_hash++;

				this->pair_count++;

				return 0;
			}
		}
	}

	log_error("?!"); // TODO
	return 1;
}
