#include "delete.h"
#include "data.h"
#include "traversal.h"
#include "hash.h"
#include "resizing.h"

#define NO_LOG_INFO

#include "../../log/log.h"

#include <inttypes.h>

int8_t hashtable_delete(void* _this, uint64_t key) {
	struct hashtable_data* this = _this;

	log_info("delete(%" PRIx64 ")", key);

	if (this->pair_count == 0) {
		// Empty hash table has no elements.
		return 1;
	}

	if (hashtable_resize_to_fit(this, this->pair_count - 1)) {
		log_error("failed to resize to fit one less element");
		return 1;
	}

	const uint64_t key_hash = hashtable_hash_of(this, key);

	struct hashtable_block *_block;
	uint8_t _subindex;
	bool _found;

	if (hashtable_find_position(this, key,
			&_block, &_subindex, &_found)) {
		return 1;
	}

	if (_found) {
		_block->occupied[_subindex] = false;
		this->blocks[key_hash].keys_with_hash--;
		this->pair_count--;

		// check_invariants(this);

		return 0;
	} else {
		log_error("key %" PRIx64 " not present, cannot delete", key);
		return 1;
	}
}
