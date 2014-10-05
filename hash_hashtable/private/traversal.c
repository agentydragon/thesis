#include "traversal.h"
#include "hash.h"

#include <string.h>

uint64_t hashtable_next_index(struct hashtable_data* this, uint64_t i) {
	return (i + 1) % this->blocks_size;
}

uint8_t hashtable_find_position(struct hashtable_data* this, uint64_t key,
		struct hashtable_block** _block, uint8_t* _subindex,
		bool* _found) {
	// Special case for empty hash tables.
	if (this->blocks_size == 0) {
		*_found = false;
		return 0;
	}

	const uint64_t key_hash = hashtable_hash_of(this, key);
	uint64_t index = key_hash;
	const uint64_t keys_with_hash = this->blocks[index].keys_with_hash;
	struct hashtable_block block;

	for (uint64_t i = 0; i < keys_with_hash; index = hashtable_next_index(this, index)) {
		// Make a local copy
		memcpy(&block, &this->blocks[index], sizeof(block));

		for (int subindex = 0; subindex < 3; subindex++) {
			const uint64_t current_key = block.keys[subindex];

			if (block.occupied[subindex]) {
				if (current_key == key) {
					*_found = true;
					*_block = &this->blocks[index];
					*_subindex = subindex;
					return 0;
				}

				if (hashtable_hash_of(this, current_key) == key_hash) {
					i++;
				}
			}
		}
		// TODO: guard against infinite loop?
	}
	*_found = false;
	return 0;
}
