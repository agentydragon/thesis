#include "htable/htable.h"

#include <inttypes.h>
#include <stdlib.h>

#include "htable/hash.h"
#include "htable/resize.h"
#include "htable/traversal.h"
#include "log/log.h"

const uint32_t HTABLE_KEYS_WITH_HASH_MAX = (1LL << 32LL) - 1;

typedef struct {
	htable_block* block;
	uint8_t slot;
} slot_pointer;

static uint8_t scan(htable* this, uint64_t key,
		slot_pointer* key_slot, slot_pointer* last_slot_with_hash,
		bool* _found) {
	*_found = false;

	// Special case for empty hash tables.
	if (this->blocks_size == 0) {
		*_found = false;
		return 0;
	}

	const uint64_t key_hash = htable_hash(this, key);
	uint64_t index = key_hash;
	const uint64_t keys_with_hash = this->blocks[index].keys_with_hash;
	htable_block current_block;

	for (uint64_t i = 0; i < keys_with_hash;
			index = htable_next_index(this, index)) {
		// Make a local copy
		current_block = this->blocks[index];

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

				if (htable_hash(this, current_key) == key_hash) {
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

int8_t htable_delete(htable* this, uint64_t key) {
	log_verbose(1, "htable_delete(%" PRIx64 ")", key);

	if (this->pair_count == 0) {
		// Empty hash table has no elements.
		return 1;
	}

	if (htable_resize_to_fit(this, this->pair_count - 1)) {
		log_error("failed to resize to fit one less element");
		return 1;
	}

	const uint64_t key_hash = htable_hash(this, key);

	slot_pointer to_delete, last;
	bool _found;

	if (scan(this, key, &to_delete, &last, &_found)) {
		return 1;
	}

	if (_found) {
		// Shorten the chain by 1.
		last.block->occupied[last.slot] = false;
		to_delete.block->keys[to_delete.slot] =
			last.block->keys[last.slot];
		to_delete.block->values[to_delete.slot] =
			last.block->values[last.slot];

		this->blocks[key_hash].keys_with_hash--;
		this->pair_count--;

		// check_invariants(this);

		return 0;
	} else {
		log_error("key %" PRIx64 " not present, cannot delete", key);
		return 1;
	}
}

void htable_find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	htable* this = _this;

	bool _found;
	slot_pointer pointer;

	ASSERT(!scan(this, key, &pointer, NULL, &_found));

	if (_found) {
		log_verbose(1, "htable_find(%" PRIu64 "): found %" PRIu64,
				key, pointer.block->values[pointer.slot]);
		*found = true;
		if (value) {
			*value = pointer.block->values[pointer.slot];
		}
	} else {
		log_verbose(1, "find(%" PRIu64 "): not found", key);
		*found = false;
	}
}
