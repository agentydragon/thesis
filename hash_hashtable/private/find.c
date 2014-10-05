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

	// Empty hash tables have undefined hashes.
	if (this->blocks_size == 0) {
		*found = false;
		return 0;
	}

	const uint64_t key_hash = hashtable_hash_of(this, key);

	log_info("find(%" PRIu64 "(h=%" PRIu64 "))", key, key_hash);

	uint64_t index = key_hash;
	const uint64_t keys_with_hash = this->blocks[index].keys_with_hash;

	struct hashtable_block block;

	for (uint64_t i = 0; i < keys_with_hash; index = hashtable_next_index(this, index)) {
		// Make a local copy
		memcpy(&block, &this->blocks[index], sizeof(block));

		for (int subindex = 0; subindex < 3; subindex++) {
			const uint64_t current_key = block.keys[subindex],
				      current_value = block.values[subindex];

			if (block.occupied[subindex]) {
				if (current_key == key) {
					*found = true;
					if (value) *value = current_value;
					log_info("find(%" PRIu64 "): found, value=%" PRIu64,
							key, current_value);
					return 0;
				}

				if (hashtable_hash_of(this, current_key) == key_hash) {
					i++;
				}
			}
		}
		// TODO: guard against infinite loop?
	}

	log_info("find(%" PRIu64 "): not found", key);
	*found = false;
	return 0;
}
