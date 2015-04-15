#include "htable/htable.h"

#include <inttypes.h>
#include <stdlib.h>

#include "htable/hash.h"
#include "htable/resize.h"
#include "htable/traversal.h"
#include "log/log.h"

const uint32_t HTABLE_KEYS_WITH_HASH_MAX = (1LL << 32LL) - 1;

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

	htable_slot_pointer to_delete, last;
	bool _found;

	if (htable_scan(this, key, &to_delete, &last, &_found)) {
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
	htable_slot_pointer pointer;

	ASSERT(!htable_scan(this, key, &pointer, NULL, &_found));

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
