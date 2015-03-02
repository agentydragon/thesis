#include "hash_hashtable/test/delete.h"
#include "hash_hashtable/test/helper.h"
#include "hash_hashtable/private/delete.h"

#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include "log/log.h"

static void assert_block_internal(const char* header, const block* found,
		const block expected) {
	for (uint8_t slot = 0; slot < 3; slot++) {
		if (found->keys[slot] != expected.keys[slot]) {
			log_fatal("%s: key %d doesn't match: %" PRIu64 " found, "
					"expected %" PRIu64, header, slot,
					found->keys[slot], expected.keys[slot]);
		}
	}
	assert(memcmp(found->values, expected.values,
				sizeof(expected.values)) == 0);
	assert(memcmp(found->occupied, expected.occupied,
				sizeof(expected.occupied)) == 0);
	assert(found->keys_with_hash == expected.keys_with_hash);
}

#define assert_block(found,expected) do { \
	assert_block_internal(#found, found, expected); \
} while (0)

static void test_shortening_chains() {
	// When we remove an element, we don't leave behind an empty hole.
	// We must do this to avoid potentially O(N) long chains.
	// So, we need to find the last pair with the same hash and put it to
	// the place where the removed element was.

	// TODO: Maybe move last pair to first empty slot? Is it a good idea?

	uint64_t hash_pairs[] = {
		10, 0,
		100, 0,
		1000, 0,

		11, 1,
		111, 1,
		1111, 1,
		11111, 1,

		22, 2,
		222, 2,
		2222, 2,

		33, 3,

		AMEN
	};

	block blocks[7] = {
		{
			.keys = { 2222, 11111, 10 },
			.values = { 11110, 55555, 50 },
			.occupied = { true, true, true },
			.keys_with_hash = 3, // 10=50, 100=500, 1000=5000
		},
		{
			.keys = { 1000, 100, 1111 },
			.values = { 5000, 500, 5555 },
			.occupied = { true, true, true },
			// 11=55, 111=555, 1111=5555, 11111=5555
			.keys_with_hash = 4,
		},
		{
			.keys = { 0, 33, 11 },
			.values = { 0, 165, 55 },
			.occupied = { false, true, true },
			// 22=110, 222=1110, 2222=11110
			.keys_with_hash = 3,
		},
		{
			.keys = { 111, 222, 22 },
			.values = { 555, 1110, 110 },
			.occupied = { true, true, true },
			// 33=165
			.keys_with_hash = 1,
		},
		// Some empty blocks to make sure deletion won't try to
		// reallocate.
		{ .keys = {}, .values = {}, .occupied = {} },
		{ .keys = {}, .values = {}, .occupied = {} },
		{ .keys = {}, .values = {}, .occupied = {} }
	};

	hashtable this = (hashtable) {
		.blocks = blocks,
		.blocks_size = 7,
		.pair_count = 11,

		.hash_fn_override = hash_mock,
		.hash_fn_override_opaque = hash_pairs
	};

	assert(hashtable_delete(&this, 11) == 0);
	assert(this.pair_count == 10);

	const block expected[4] = {
		{
			// 11111=55555 got moved to block 2.
			.keys = { 2222, 11111, 10 },
			.values = { 11110, 55555, 50 },
			.occupied = { true, false, true },
			.keys_with_hash = 3
		},
		{
			// keys_with_hash--, because 11111=55555 got deleted.
			.keys = { 1000, 100, 1111 },
			.values = { 5000, 500, 5555 },
			.occupied = { true, true, true },
			.keys_with_hash = 3
		},
		{
			// 11111=55555 was moved to where 11=55 was.
			.keys = { 0, 33, 11111 },
			.values = { 0, 165, 55555 },
			.occupied = { false, true, true },
			.keys_with_hash = 3,
		},
		{
			// No change.
			.keys = { 111, 222, 22 },
			.values = { 555, 1110, 110 },
			.occupied = { true, true, true },
			.keys_with_hash = 1,
		}
	};
	assert_block(&blocks[0], expected[0]);
	assert_block(&blocks[1], expected[1]);
	assert_block(&blocks[2], expected[2]);
	assert_block(&blocks[3], expected[3]);
}

void hashtable_test_delete() {
	test_shortening_chains();
}
