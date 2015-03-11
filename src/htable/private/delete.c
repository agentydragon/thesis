#include "htable/private/delete.h"

#include <inttypes.h>

#define NO_LOG_INFO

#include "log/log.h"

#include "htable/private/data.h"
#include "htable/private/hash.h"
#include "htable/private/resizing.h"
#include "htable/private/traversal.h"

int8_t htable_delete(htable* this, uint64_t key) {
	log_info("delete(%" PRIx64 ")", key);

	if (this->pair_count == 0) {
		// Empty hash table has no elements.
		return 1;
	}

	if (htable_resize_to_fit(this, this->pair_count - 1)) {
		log_error("failed to resize to fit one less element");
		return 1;
	}

	const uint64_t key_hash = hash_of(this, key);

	struct htable_slot_pointer to_delete, last;
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
