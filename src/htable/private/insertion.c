#include "htable/private/insertion.h"

#include <inttypes.h>

#include "log/log.h"
#include "htable/private/hash.h"
#include "htable/private/traversal.h"
#include "htable/private/helper.h"

int8_t htable_insert_internal(htable* this, uint64_t key, uint64_t value) {
	const uint64_t key_hash = hash_of(this, key);
	block* const home_block = &this->blocks[key_hash];

	if (home_block->keys_with_hash == HTABLE_KEYS_WITH_HASH_MAX) {
		log_error("cannot insert - overflow in maximum bucket size");
		return 1;
	}

	for (uint64_t i = 0, index = key_hash, traversed = 0;
			traversed < this->blocks_size;
			index = htable_next_index(this, index), traversed++) {
		block* current_block = &this->blocks[index];

		for (int8_t slot = 0; slot < 3; slot++) {
			if (current_block->occupied[slot]) {
				if (current_block->keys[slot] == key) {
					log_error("duplicate in bucket %" PRIu64 " "
						"when inserting %" PRIu64 "=%" PRIu64,
						index, key, value);
					return 1;
				}

				if (hash_of(this, current_block->keys[slot]) == key_hash) {
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
