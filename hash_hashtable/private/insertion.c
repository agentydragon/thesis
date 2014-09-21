#include "insertion.h"
#include "hash.h"
#include "traversal.h"

#include <inttypes.h>

#include "../../log/log.h"

int8_t hashtable_insert_internal(struct hashtable_data* this, uint64_t key, uint64_t value) {
	uint64_t key_hash = hash_of(this, key);
	if (this->table[key_hash].keys_with_hash == HASHTABLE_KEYS_WITH_HASH_MAX) {
		log_error("cannot insert - overflow in maximum bucket size");
		return 1;
	}

	bool free_index_found = false;
	uint64_t free_index;

	for (uint64_t i = 0, index = key_hash;
		i < this->table[key_hash].keys_with_hash || !free_index_found;
		index = hashtable_next_index(this, index)\
	) {

		if (this->table[index].occupied) {
			if (hash_of(this, this->table[index].key) == key_hash) {
				i++;

				if (this->table[index].key == key) {
					log_error("duplicate in bucket %" PRIu64 " when inserting %" PRIu64 "=%" PRIu64, index, key, value);
					return 1;
				}
			}
		} else {
			if (!free_index_found) {
				free_index = index;
				free_index_found = true;
			}
		}
	}

	this->table[free_index].occupied = true;
	this->table[free_index].key = key;
	this->table[free_index].value = value;

	this->table[key_hash].keys_with_hash++;

	this->pair_count++;

	// check_invariants(this);

	return 0;
}
