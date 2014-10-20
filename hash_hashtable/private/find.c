#include "find.h"
#include "data.h"
#include "hash.h"
#include "traversal.h"

#define NO_LOG_INFO

#include "../../log/log.h"

#include <string.h>
#include <inttypes.h>

int8_t hashtable_find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	struct hashtable_data* this = _this;

	bool _found;
	struct hashtable_slot_pointer pointer;

	if (hashtable_scan(this, key, &pointer, NULL, &_found)) {
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
