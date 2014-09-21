#ifndef HASH_HASHTABLE_PRIVATE_DATA_H
#define HASH_HASHTABLE_PRIVATE_DATA_H

#include <stdint.h>
#include <stdbool.h>

const uint32_t HASHTABLE_KEYS_WITH_HASH_MAX;

// Takes 3x8 bytes
struct hashtable_bucket {
	// how many keys have this hash?
	uint32_t keys_with_hash;

	// the key stored in this bucket
	bool occupied;

	uint64_t key;
	uint64_t value;
};

struct hashtable_data {
	struct hashtable_bucket* table;
	uint64_t table_size;
	uint64_t pair_count;
};

#endif
