#include "test.h"
#include "ordered_file_maintenance.h"
#include "../log/log.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>

#define UNDEF 0xDEADBEEF
#define NOTHING 0xDEADDEADDEADDEAD

#define assert_contents(subrange,...) do { \
	const uint64_t _expected[] = { __VA_ARGS__ }; \
	const uint64_t size = sizeof(_expected) / sizeof(*_expected); \
	for (uint64_t i = 0; i < size; i++) { \
		if (_expected[i] == NOTHING) { \
			if (subrange.occupied[i]) { \
				log_fatal("index %" PRIu64 " should be empty, " \
						"but is occupied by %" PRIu64, \
						i, subrange.contents[i]); \
			} \
		} else { \
			if (!subrange.occupied[i]) { \
				log_fatal("index %" PRIu64 " should be occupied by " \
						"%" PRIu64 ", but it's empty", \
						i, _expected[i]); \
			} \
			assert(subrange.contents[i] == _expected[i]); \
		} \
	} \
} while (0)

static void test_insert_simple() {
	bool occupied[] = { true, false, false, false, false };
	uint64_t contents[] = { 100, 0, 0, 0, 0 };
	struct subrange subrange = {
		.occupied = occupied,
		.contents = contents,
		.size = 5
	};
	assert(subrange_insert_after(subrange, 200, 100));

	assert_contents(subrange, 100, 200, NOTHING, NOTHING, NOTHING);
}

static void test_shift() {
	bool occupied[] = { true, true, true, false, false };
	uint64_t contents[] = { 100, 300, 400, 0, 0 };
	struct subrange subrange = {
		.occupied = occupied,
		.contents = contents,
		.size = 5
	};
	assert(subrange_insert_after(subrange, 200, 100));

	assert_contents(subrange, 100, 200, 300, 400, NOTHING);
}

void test_insert_to_full() {
	bool occupied[] = { true, true, true, true, true };
	uint64_t contents[] = { 0, 0, 100, 0, 0 };
	struct subrange subrange = {
		.occupied = occupied,
		.contents = contents,
		.size = 5
	};
	assert(!subrange_insert_after(subrange, 200, 100));
}

static void test_subrange_insert_after() {
	test_insert_simple();
	test_shift();
	test_insert_to_full();
}

void test_subrange_delete() {
	bool occupied[] = { true, true, true, true, false };
	uint64_t contents[] = { 10, 20, 30, 40, 0 };
	struct subrange subrange = {
		.occupied = occupied,
		.contents = contents,
		.size = 5
	};
	subrange_delete(subrange, 30);

	assert_contents(subrange, 10, 20, NOTHING, 40, NOTHING);
}

static void test_subrange_compact() {
	bool occupied[] = { true, true, false, false, false, false, false, true };
	uint64_t contents[] = { 10, 20, 0, 0, 0, 0, 0, 30 };
	struct subrange subrange = {
		.occupied = occupied,
		.contents = contents,
		.size = 8
	};
	uint64_t new_location;
	subrange_compact(subrange, (struct watched_index) {
		.index = 7,
		.new_location = &new_location
	});

	assert(new_location == 2);
	assert_contents(subrange, 10, 20, 30,
			NOTHING, NOTHING, NOTHING, NOTHING);
}

static void test_subrange_spread_evenly() {
	bool occupied[] = { true, true, false, false, false, false, false, true };
	uint64_t contents[] = { 10, 20, 0, 0, 0, 0, 0, 30 };
	struct subrange subrange = {
		.occupied = occupied,
		.contents = contents,
		.size = 8
	};
	uint64_t new_location;
	subrange_spread_evenly(subrange, (struct watched_index) {
		.index = 1,
		.new_location = &new_location
	});

	assert(new_location == 4);
	assert_contents(subrange,
			NOTHING, NOTHING, 10, NOTHING,
			20, NOTHING, NOTHING, 30);
}

static void test_insert_after_two_reorg() {
	// This rest reorganizes 1 block.
	bool occupied[] = {
		true, true, true, true,
		false, false, true, true,
		false, false, true, true,
		false, false, true, true,
		false, false, true, true,
		false, false, false, false,
		false, false, true, true,
		true, false, true, false,
	};
	uint64_t contents[] = {
		100, 200, 300, 400,
		UNDEF, UNDEF, 500, 600,
		UNDEF, UNDEF, 700, 800,
		UNDEF, UNDEF, 900, 1000,
		UNDEF, UNDEF, 1100, 1200,
		UNDEF, UNDEF, UNDEF, UNDEF,
		UNDEF, UNDEF, 1300, 1400,
		1500, UNDEF, 1600, UNDEF
	};
	struct ordered_file file = {
		.occupied = occupied,
		.contents = contents,
		.capacity = 32
	};
	ordered_file_insert_after(file, 250, 1);

	struct subrange ranged = {
		.occupied = occupied,
		.contents = contents,
		.size = 32
	};
	assert_contents(ranged,
			// The following block is changed.
			100, 200, 250, NOTHING,
			300, 400, NOTHING, 500,
			NOTHING, 600, 700, NOTHING,
			800, 900, NOTHING, 1000,
			// No changes below this point.
			NOTHING, NOTHING, 1100, 1200,
			NOTHING, NOTHING, NOTHING, NOTHING,
			NOTHING, NOTHING, 1300, 1400,
			1500, NOTHING, 1600, NOTHING);
}

static void test_insert_after_very_dense() {
	// After insertion, we will have 24 elements, which is 3/4 of 32.
	bool occupied[] = {
		false, false, true, false,
		false, false, false, false,
		true, true, true, true,
		true, true, false, true,
		true, true, true, true,
		true, true, true, true,
		true, true, false, true,
		true, true, true, true
	};
	uint64_t contents[] = {
		UNDEF, UNDEF, 100, UNDEF,
		UNDEF, UNDEF, UNDEF, UNDEF,
		200, 300, 400, 500,
		600, 700, UNDEF, 800,
		900, 1000, 1100, 1200,
		1300, 1400, 1500, 1600,
		1700, 1800, UNDEF, 1900,
		2000, 2100, 2200, 2300
	};
	struct ordered_file file = {
		.occupied = occupied,
		.contents = contents,
		.capacity = 32
	};
	ordered_file_insert_after(file, 2400, 31);

	struct subrange ranged = {
		.occupied = occupied,
		.contents = contents,
		.size = 32
	};
	char description[512];
	subrange_describe(ranged, description);
	log_info("%s", description);
	assert_contents(ranged,
			// The whole structure is reorged.
			100, NOTHING, 200, 300,
			NOTHING, 400, 500, 600,
			NOTHING, 700, 800, NOTHING,
			900, 1000, 1100, NOTHING,
			1200, 1300, 1400, NOTHING,
			1500, 1600, NOTHING, 1700,
			1800, 1900, NOTHING, 2000,
			2100, 2200, 2300, 2400);
}


static void test_insert_after() {
	test_insert_after_two_reorg();
	test_insert_after_very_dense();
}

void test_ordered_file_maintenance() {
	test_subrange_insert_after();
	test_subrange_delete();
	test_subrange_compact();
	test_subrange_spread_evenly();
	test_insert_after();
}
