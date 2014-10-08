#include "find.h"
#include "../private/data.h"
#include "../private/find.h"
#include "../../log/log.h"

#include <assert.h>
#include <stdlib.h>

static uint64_t AMEN = 0xFFFFFFFFFFFFFFFF;

static uint64_t hash_mock(void* _pairs, uint64_t key) {
	uint64_t* key_ptr = _pairs;

	do {
		if (*key_ptr == key) return *(key_ptr + 1);
		if (*key_ptr == AMEN) {
			log_fatal("hash_mock passed undefined key %ld\n", key);
			return -1;
		}
		key_ptr += 2;
	} while (true);
}

static void assert_found(struct hashtable_data* hashtable, uint64_t key, uint64_t value) {
	uint64_t _value;
	bool _found;
	assert(hashtable_find(hashtable, key, &_value, &_found) == 0);
	assert(_found && _value == value);
}

static void assert_not_found(struct hashtable_data* hashtable, uint64_t key) {
	bool _found;
	assert(hashtable_find(hashtable, key, NULL, &_found) == 0);
	assert(!_found);
}

void hashtable_test_find() {
	uint64_t hash_pairs[] = {
		100, 1,
		101, 1,
		102, 1,
		103, 1,
		104, 1,
		200, 2,
		300, 3,
		301, 3,
		302, 3,
		AMEN
	};

	struct hashtable_block blocks[4] = {
		{ .keys = {}, .values = {}, .occupied = {} },
		{
			.keys = { 302, 101, 100 },
			.values = { 4, 5, 6 },
			.occupied = { true, true, true },
			.keys_with_hash = 4, // hash = 1
		},
		{
			.keys = { 200, -1, 103 },
			.values = { 8, 9, 10 },
			.occupied = { true, false, true },
			.keys_with_hash = 1, // hash = 2
		},
		{
			.keys = { 300, 104, 301 },
			.values = { 11, 42, -1 },
			.occupied = { true, true, false },
			.keys_with_hash = 2 // hash = 3
		}
	};

	struct hashtable_data hashtable = {
		.blocks = blocks,
		.blocks_size = 4,
		.pair_count = 7,

		.hash_fn_override = hash_mock,
		.hash_fn_override_opaque = hash_pairs
	};

	assert_found(&hashtable, 302, 4);
	assert_found(&hashtable, 101, 5);
	assert_found(&hashtable, 100, 6);
	assert_found(&hashtable, 200, 8);
	assert_found(&hashtable, 103, 10);
	assert_found(&hashtable, 300, 11);
	assert_found(&hashtable, 104, 42);
	assert_not_found(&hashtable, 301);
}
