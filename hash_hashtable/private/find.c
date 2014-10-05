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

	struct hashtable_block *_block;
	uint8_t _subindex;
	bool _found;

	if (hashtable_find_position(this, key,
			&_block, &_subindex, &_found)) {
		return 1;
	}

	if (_found) {
		log_info("find(%" PRIu64 "): found, value=%" PRIu64,
				key, _block->values[_subindex]);
		*found = true;
		if (value) *value = _block->values[_subindex];
	} else {
		log_info("find(%" PRIu64 "): not found", key);
		*found = false;
	}
	return 0;
}
