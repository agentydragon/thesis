#include "htable/private/find.h"

#include <string.h>
#include <inttypes.h>

#include "htable/private/data.h"
#include "htable/private/hash.h"
#include "htable/private/traversal.h"

#define NO_LOG_INFO
#include "log/log.h"

int8_t htable_find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	htable* this = _this;

	bool _found;
	struct htable_slot_pointer pointer;

	if (htable_scan(this, key, &pointer, NULL, &_found)) {
		return 1;
	}

	if (_found) {
		log_info("find(%" PRIu64 "): found, value=%" PRIu64,
				key, pointer.block->values[pointer.slot]);
		*found = true;
		if (value) *value = pointer.block->values[pointer.slot];
	} else {
		log_info("find(%" PRIu64 "): not found", key);
		*found = false;
	}
	return 0;
}
