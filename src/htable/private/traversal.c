#include "traversal.h"
#include "hash.h"
#include "helper.h"

#include <string.h>

uint64_t htable_next_index(const htable* this, uint64_t i) {
	return (i + 1) % this->blocks_size;
}

uint8_t htable_scan(htable* this, uint64_t key,
		slot_pointer* key_slot, slot_pointer* last_slot_with_hash,
		bool* _found) {
	*_found = false;

	// Special case for empty hash tables.
	if (this->blocks_size == 0) {
		*_found = false;
		return 0;
	}

	const uint64_t key_hash = hash_of(this, key);
	uint64_t index = key_hash;
	const uint64_t keys_with_hash = this->blocks[index].keys_with_hash;
	block current_block;

	for (uint64_t i = 0; i < keys_with_hash;
			index = htable_next_index(this, index)) {
		// Make a local copy
		memcpy(&current_block, &this->blocks[index], sizeof(block));

		for (uint8_t slot = 0; slot < 3; slot++) {
			const uint64_t current_key = current_block.keys[slot];

			if (current_block.occupied[slot]) {
				if (current_key == key) {
					*_found = true;

					if (key_slot != NULL) {
						key_slot->block = &this->blocks[index];
						key_slot->slot = slot;
					}

					if (last_slot_with_hash == NULL) {
						// We don't care and we're done.
						return 0;
					}
				}

				if (hash_of(this, current_key) == key_hash) {
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

	return 0;
}
