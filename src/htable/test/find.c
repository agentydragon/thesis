#include "htable/test/find.h"
#include "htable/test/helper.h"

#include <assert.h>
#include <stdlib.h>

#include "log/log.h"

static void assert_found(htable* this, uint64_t key, uint64_t value) {
	uint64_t _value;
	bool _found;
	ASSERT(htable_find(this, key, &_value, &_found) == 0);
	ASSERT(_found && _value == value);
}

static void assert_not_found(htable* this, uint64_t key) {
	bool _found;
	ASSERT(htable_find(this, key, NULL, &_found) == 0);
	ASSERT(!_found);
}

void htable_test_find() {
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

	htable_block blocks[4] = {
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

	htable this = {
		.blocks = blocks,
		.blocks_size = 4,
		.pair_count = 7,

		.hash_fn_override = hash_mock,
		.hash_fn_override_opaque = hash_pairs
	};

	assert_found(&this, 302, 4);
	assert_found(&this, 101, 5);
	assert_found(&this, 100, 6);
	assert_found(&this, 200, 8);
	assert_found(&this, 103, 10);
	assert_found(&this, 300, 11);
	assert_found(&this, 104, 42);
	assert_not_found(&this, 301);
}
