#include "test.h"
#include "ordered_file_maintenance.h"
#include "../log/log.h"
#include "../math/math.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNDEF 0xDEADBEEF
#define NIL 0xDEADDEADDEADDEAD

static void _assert_keys(struct ordered_file file,
		struct ordered_file_range range,
		const uint64_t *_expected, uint64_t size) {
	for (uint64_t i = 0; i < size; i++) {
		if (_expected[i] == NIL) {
			CHECK(!file.occupied[range.begin + i],
					"index %" PRIu64 " should be empty, "
					"but is occupied by %" PRIu64,
					i, file.keys[range.begin + i]);
		} else {
			CHECK(file.occupied[range.begin + i],
					"index %" PRIu64 " should contain "
					"%" PRIu64 ", but it's empty",
					i, _expected[i]);
			CHECK(file.keys[range.begin + i] == _expected[i],
					"key %" PRIu64 " should be %" PRIu64
					", but it is %" PRIu64, i, _expected[i],
					file.keys[range.begin + i]);
		}
	}
}

#define assert_keys(file,range,...) do { \
	const uint64_t _expected[] = { __VA_ARGS__ }; \
	const uint64_t size = sizeof(_expected) / sizeof(*_expected); \
	_assert_keys(file, range, _expected, size); \
} while (0)

#define assert_file(file,...) do { \
	const uint64_t _expected[] = { __VA_ARGS__ }; \
	const uint64_t size = sizeof(_expected) / sizeof(*_expected); \
	struct ordered_file_range range = { \
		.begin = 0, \
		.size = size \
	}; \
	_assert_keys(file, range, _expected, size); \
} while (0)

// Used for debugging.
static void __attribute__((unused)) dump_file(struct ordered_file file) {
	log_info("capacity=%" PRIu64 " block_size=%" PRIu64,
			file.parameters.capacity,
			file.parameters.block_size);
	char buffer[256];
	uint64_t offset = 0;
	for (uint64_t i = 0; i < file.parameters.capacity; i++) {
		if (i % file.parameters.block_size == 0 && i != 0) {
			offset += sprintf(buffer + offset, "\n");
		}
		if (file.occupied[i]) {
			offset += sprintf(buffer + offset, "%3" PRIu64 " ",
					file.keys[i]);
		} else {
			offset += sprintf(buffer + offset, "--- ");
		}
	}
	log_info("keys={%s}", buffer);
}

static void test_insert_simple() {
	bool occupied[] = { true, false, false, false, false };
	uint64_t keys[] = { 100, 0, 0, 0, 0 };
	struct ordered_file file = {
		.occupied = occupied,
		.keys = keys,
	};
	const struct ordered_file_range range = {
		.begin = 0,
		.size = 5
	};
	range_insert_after(file, range, 200, 0);

	assert_keys(file, range, 100, 200, NIL, NIL, NIL);
}

static void test_shift() {
	bool occupied[] = { true, true, true, false, false };
	uint64_t keys[] = { 100, 300, 400, 0, 0 };
	struct ordered_file file = {
		.occupied = occupied,
		.keys = keys,
	};
	const struct ordered_file_range range = {
		.begin = 0,
		.size = 5
	};
	range_insert_after(file, range, 200, 0);

	assert_keys(file, range, 100, 200, 300, 400, NIL);
}

static void test_range_insert_after() {
	test_insert_simple();
	test_shift();
}

static void test_range_compact() {
	bool occupied[] = { true, true, false, false, false, false, false, true };
	uint64_t keys[] = { 10, 20, 0, 0, 0, 0, 0, 30 };
	struct ordered_file file = {
		.occupied = occupied,
		.keys = keys,
	};
	const struct ordered_file_range range = {
		.begin = 0,
		.size = 8
	};
	uint64_t new_location;
	range_compact(file, range, (struct watched_index) {
		.index = 7,
		.new_location = &new_location
	});

	assert(new_location == 2);
	assert_keys(file, range, 10, 20, 30,
			NIL, NIL, NIL, NIL);
}

static void test_range_spread_evenly() {
	bool occupied[] = { true, true, false, false, false, false, false, true };
	uint64_t keys[] = { 10, 20, 0, 0, 0, 0, 0, 30 };
	struct ordered_file file = {
		.occupied = occupied,
		.keys = keys,
	};
	const struct ordered_file_range range = {
		.begin = 0,
		.size = 8
	};
	uint64_t new_location;
	range_spread_evenly(file, range, (struct watched_index) {
		.index = 1,
		.new_location = &new_location
	});

	assert(new_location == 4);
	assert_keys(file, range,
			NIL, NIL, 10, NIL,
			20, NIL, NIL, 30);
}

#define COUNTOF(x) (sizeof(x) / sizeof(*(x)))

#define make_file(_file,_block_size,...) do { \
	const uint64_t _values[] = { __VA_ARGS__ }; \
	const uint64_t _count = COUNTOF(_values); \
	struct ordered_file* __file = (_file); \
	__file->occupied = malloc(sizeof(bool) * _count); \
	__file->keys = malloc(sizeof(uint64_t) * _count); \
	__file->parameters.capacity = _count; \
	__file->parameters.block_size = _block_size; \
	for (uint64_t i = 0; i < _count; i++) { \
		if (_values[i] == NIL) { \
			__file->occupied[i] = false; \
			__file->keys[i] = UNDEF; \
		} else { \
			__file->occupied[i] = true; \
			__file->keys[i] = _values[i]; \
		} \
	} \
} while (0)

static void destroy_file(struct ordered_file file) {
	free(file.occupied);
	free(file.keys);
}

static void test_insert_after_two_reorg() {
	// This rest reorganizes 1 block.
	struct ordered_file file;
	make_file(&file, 4,
			100, 200, 300, 400,
			NIL, NIL, 500, 600,
			NIL, NIL, 700, 800,
			NIL, NIL, 900, 1000,
			NIL, NIL, 1100, 1200,
			NIL, NIL, NIL, NIL,
			NIL, NIL, 1300, 1400,
			1500, NIL, 1600, NIL);
	ordered_file_insert_after(&file, 250, 1);

	dump_file(file);
	assert_file(file,
			// The following block is changed.
			100, 200, 250, NIL,
			300, 400, NIL, 500,
			NIL, 600, 700, NIL,
			800, 900, NIL, 1000,
			// The following is unchanged.
			NIL, NIL, 1100, 1200,
			NIL, NIL, NIL, NIL,
			NIL, NIL, 1300, 1400,
			1500, NIL, 1600, NIL);
	destroy_file(file);
}

static void test_insert_after_very_dense() {
	// After insertion, we will have 24 elements, which is 3/4 of 32.
	struct ordered_file file;
	make_file(&file, 4,
			NIL, NIL, 100, NIL,
			NIL, NIL, NIL, NIL,
			200, 300, 400, 500,
			600, 700, NIL, 800,
			900, 1000, 1100, 1200,
			1300, 1400, 1500, 1600,
			1700, 1800, NIL, 1900,
			2000, 2100, 2200, 2300);
	ordered_file_insert_after(&file, 2400, 31);

	assert_file(file,
			// The whole structure is reorged.
			100, NIL, 200, 300,
			NIL, 400, 500, 600,
			NIL, 700, 800, NIL,
			900, 1000, 1100, NIL,
			1200, 1300, 1400, NIL,
			1500, 1600, NIL, 1700,
			1800, 1900, NIL, 2000,
			2100, 2200, 2300, 2400);
	destroy_file(file);
}

static void test_comprehensive_resizing() {
	struct ordered_file file;
	make_file(&file, 4,
			100, NIL, 200, 300,
			400, 500, 600, NIL,
			NIL, 700, 800, 900,
			1000, 1100, 1200, 1300);
	ordered_file_insert_after(&file, 1242, 14);

	// 14/20 = 0.7, within bounds.
	assert(file.parameters.block_size == 5);
	assert(file.parameters.capacity == 20);

	assert_file(file,
			100, NIL, 200, 300, NIL,
			400, 500, NIL, 600, 700,
			NIL, 800, 900, NIL, 1000,
			1100, 1200, 1242, 1300, NIL);

	ordered_file_insert_after(&file, 942, 12);  // 942 after 900
	ordered_file_insert_first(&file, 42);
	ordered_file_insert_first(&file, 15);

	// 17/24 = 0.70833.., within bounds.
	assert(file.parameters.block_size == 6);
	assert(file.parameters.capacity == 24);
	assert_file(file,
			15, 42, 100, 200, 300, NIL,
			400, NIL, 500, 600, NIL, 700,
			800, NIL, 900, 942, NIL, 1000,
			1100, NIL, 1200, 1242, NIL, 1300);

	ordered_file_insert_after(&file, 17, 0);  // 17 after 15
	ordered_file_insert_after(&file, 16, 0);  // 16 after 15
	ordered_file_insert_after(&file, 1142, 19);  // 1142 after 1100

	// 20/32 = 0.625, within bounds.
	assert(file.parameters.block_size == 4);
	assert(file.parameters.capacity == 32);
	assert_file(file,
			15, NIL, 16, 17,
			NIL, 42, NIL, 100,
			200, NIL, 300, NIL,
			400, 500, NIL, 600,
			NIL, 700, NIL, 800,
			900, NIL, 942, NIL,
			1000, 1100, 1142, 1200,
			NIL, 1242, 1300, NIL);

	ordered_file_delete(&file, 0);  // deletes 15
	ordered_file_delete(&file, 30);  // deletes 1300
	ordered_file_delete(&file, 31);  // deletes 1242
	assert_file(file,
			NIL, 16, NIL, 17,
			NIL, 42, NIL, 100,
			200, NIL, 300, NIL,
			400, 500, NIL, 600,
			NIL, 700, NIL, 800,
			900, NIL, 942, NIL,
			/* touched => */ NIL, 1000, NIL, 1100,
			/* touched => */ NIL, 1142, NIL, 1200);

	ordered_file_delete(&file, 7);  // deletes 100
	ordered_file_delete(&file, 8);  // deletes 200
	ordered_file_delete(&file, 11);  // deletes 300
	ordered_file_delete(&file, 10);  // deletes 400

	assert(file.parameters.block_size == 5);
	assert(file.parameters.capacity == 20);
	assert_file(file,
			16, NIL, 17, 42, NIL,
			500, 600, NIL, 700, 800,
			NIL, 900, 942, NIL, 1000,
			1100, NIL, 1142, 1200, NIL);

	ordered_file_delete(&file, 5);  // deletes 500
	ordered_file_delete(&file, 6);  // deletes 600
	assert_file(file,
			NIL, 16, 17, NIL, 42,
			NIL, 700, NIL, NIL, 800,
			NIL, 900, 942, NIL, 1000,
			1100, NIL, 1142, 1200, NIL);

	ordered_file_delete(&file, 15);  // deletes 1100
	ordered_file_delete(&file, 19);  // deletes 1200
	ordered_file_delete(&file, 9);  // deletes 800
	assert_file(file,
			NIL, 16, NIL, NIL, 17,
			NIL, 42, NIL, NIL, 700,
			NIL, 900, NIL, NIL, 942,
			NIL, 1000, NIL, NIL, 1142);

	ordered_file_delete(&file, 16);  // deletes 1000

	assert(file.parameters.block_size == 5);
	assert(file.parameters.capacity == 10);
	assert_file(file,
			16, 17, NIL, 42, 700,
			NIL, 900, 942, NIL, 1142);

	ordered_file_delete(&file, 4);  // deletes 700
	ordered_file_delete(&file, 6);  // deletes 900
	ordered_file_delete(&file, 1);  // deletes 16
	assert_file(file,
			NIL, 17, NIL, NIL, 42,
			NIL, 942, NIL, NIL, 1142);

	ordered_file_delete(&file, 1);  // deletes 17
	assert(file.parameters.block_size == 4);
	assert(file.parameters.capacity == 4);
	assert_file(file, 42, 942, NIL, 1142);

	destroy_file(file);
}

static void test_insert_after() {
	test_insert_after_two_reorg();
	test_insert_after_very_dense();
}

static void test_parameter_policy() {
	for (uint64_t N = 0; N < 100000; N++) {
		struct parameters parameters = adequate_parameters(N);
		CHECK(parameters.capacity % parameters.block_size == 0,
				"Capacity for %" PRIu64 " (%" PRIu64 ") is "
				"not divisible by block size (%" PRIu64 ")",
				N, parameters.capacity, parameters.block_size);
		CHECK(global_density_within_threshold(N, parameters.capacity),
				"Capacity for %" PRIu64 " (%" PRIu64 ") does "
				"not meet global thresholds.",
				N, parameters.capacity);
	}
}

void test_ordered_file_maintenance() {
	test_parameter_policy();

	test_range_insert_after();
	test_range_compact();
	test_range_spread_evenly();
	test_insert_after();

	test_comprehensive_resizing();
}
