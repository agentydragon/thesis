#include "test.h"
#include "ordered_file_maintenance.h"
#include "../log/log.h"
#include "../math/math.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_helpers.h"

static void _assert_keys(struct ordered_file file,
		struct ordered_file_range range,
		const uint64_t *_expected, uint64_t size) {
	for (uint64_t i = 0; i < size; i++) {
		const uint64_t index = range.begin + i;
		if (_expected[i] == NIL) {
			CHECK(!file.occupied[index],
					"index %" PRIu64 " should be empty, "
					"but is occupied by %" PRIu64,
					i, file.items[index].key);
		} else {
			CHECK(file.occupied[index],
					"index %" PRIu64 " should contain "
					"%" PRIu64 ", but it's empty",
					i, _expected[i]);
			CHECK(file.items[index].key == _expected[i],
					"key at %" PRIu64 " should be %" PRIu64
					", but it is %" PRIu64, i, _expected[i],
					file.items[index].key);
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

#define MAKE_FILE(...) struct ordered_file file; make_file(&file, __VA_ARGS__);
#define INSERT_AFTER(_key,index) do { \
	ordered_file_insert_after(&file, (ordered_file_item) { \
		.key = _key \
	}, index); \
} while (0)
#define INSERT_FIRST(_key) do { \
	ordered_file_insert_first(&file, (ordered_file_item) { \
		.key = _key \
	}); \
} while (0)
#define DELETE_AT(index) ordered_file_delete(&file, index)
#define ASSERT_FILE(...) assert_file(file, __VA_ARGS__)

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
					file.items[i].key);
		} else {
			offset += sprintf(buffer + offset, "--- ");
		}
	}
	log_info("items={%s}", buffer);
}

static void test_insert_simple() {
	MAKE_FILE(5,
			100, NIL, NIL, NIL, NIL);
	const struct ordered_file_range range = {
		.begin = 0,
		.size = 5
	};
	range_insert_after(file, range, (ordered_file_item) {
		.key = 200
	}, 0);

	assert_keys(file, range, 100, 200, NIL, NIL, NIL);
	destroy_file(file);
}

static void test_shift() {
	MAKE_FILE(5,
			100, 300, 400, NIL, NIL);
	const struct ordered_file_range range = {
		.begin = 0,
		.size = 5
	};
	range_insert_after(file, range, (ordered_file_item) {
		.key = 200
	}, 0);

	assert_keys(file, range, 100, 200, 300, 400, NIL);
	destroy_file(file);
}

static void test_range_insert_after() {
	test_insert_simple();
	test_shift();
}

static void test_range_compact() {
	MAKE_FILE(4,
			10, 20, NIL, NIL,
			NIL, NIL, NIL, 30);
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
	destroy_file(file);
}

static void test_range_spread_evenly() {
	MAKE_FILE(4,
			10, 20, NIL, NIL,
			NIL, NIL, NIL, 30);
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
	destroy_file(file);
}

static void test_insert_after_two_reorg() {
	// This rest reorganizes 1 block.
	MAKE_FILE(4,
			100, 200, 300, 400,
			NIL, NIL, 500, 600,
			NIL, NIL, 700, 800,
			NIL, NIL, 900, 1000,
			NIL, NIL, 1100, 1200,
			NIL, NIL, NIL, NIL,
			NIL, NIL, 1300, 1400,
			1500, NIL, 1600, NIL);
	INSERT_AFTER(250, 1);

	ASSERT_FILE(
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
	INSERT_AFTER(2400, 31);

	ASSERT_FILE(
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
	INSERT_AFTER(1242, 14);

	// 14/20 = 0.7, within bounds.
	assert(file.parameters.block_size == 5);
	assert(file.parameters.capacity == 20);

	ASSERT_FILE(
			100, NIL, 200, 300, NIL,
			400, 500, NIL, 600, 700,
			NIL, 800, 900, NIL, 1000,
			1100, 1200, 1242, 1300, NIL);

	INSERT_AFTER(942, 12);  // 942 after 900
	INSERT_FIRST(42);
	INSERT_FIRST(15);

	// 17/24 = 0.70833.., within bounds.
	assert(file.parameters.block_size == 6);
	assert(file.parameters.capacity == 24);
	ASSERT_FILE(
			15, 42, 100, 200, 300, NIL,
			400, NIL, 500, 600, NIL, 700,
			800, NIL, 900, 942, NIL, 1000,
			1100, NIL, 1200, 1242, NIL, 1300);

	INSERT_AFTER(17, 0);  // 17 after 15
	INSERT_AFTER(16, 0);  // 16 after 15
	INSERT_AFTER(1142, 19);  // 1142 after 1100

	// 20/32 = 0.625, within bounds.
	assert(file.parameters.block_size == 4);
	assert(file.parameters.capacity == 32);
	ASSERT_FILE(
			15, NIL, 16, 17,
			NIL, 42, NIL, 100,
			200, NIL, 300, NIL,
			400, 500, NIL, 600,
			NIL, 700, NIL, 800,
			900, NIL, 942, NIL,
			1000, 1100, 1142, 1200,
			NIL, 1242, 1300, NIL);

	DELETE_AT(0);  // deletes 15
	DELETE_AT(30);  // deletes 1300
	DELETE_AT(29);  // deletes 1242
	ASSERT_FILE(
			NIL, NIL, 16, 17,
			NIL, 42, NIL, 100,
			200, NIL, 300, NIL,
			400, 500, NIL, 600,
			NIL, 700, NIL, 800,
			900, NIL, 942, NIL,
			/* touched => */ NIL, 1000, NIL, 1100,
			/* touched => */ NIL, 1142, NIL, 1200);

	DELETE_AT(7);  // deletes 100
	DELETE_AT(8);  // deletes 200
	DELETE_AT(10);  // deletes 300
	DELETE_AT(10);  // deletes 400

	assert(file.parameters.block_size == 5);
	assert(file.parameters.capacity == 20);
	ASSERT_FILE(
			16, NIL, 17, 42, NIL,
			500, 600, NIL, 700, 800,
			NIL, 900, 942, NIL, 1000,
			1100, NIL, 1142, 1200, NIL);

	DELETE_AT(5);  // deletes 500
	DELETE_AT(6);  // deletes 600
	ASSERT_FILE(
			NIL, 16, 17, NIL, 42,
			NIL, 700, NIL, NIL, 800,
			NIL, 900, 942, NIL, 1000,
			1100, NIL, 1142, 1200, NIL);

	DELETE_AT(15);  // deletes 1100
	DELETE_AT(18);  // deletes 1200
	DELETE_AT(9);  // deletes 800
	ASSERT_FILE(
			NIL, 16, NIL, NIL, 17,
			NIL, 42, NIL, NIL, 700,
			NIL, 900, NIL, NIL, 942,
			NIL, 1000, NIL, NIL, 1142);

	DELETE_AT(16);  // deletes 1000

	assert(file.parameters.block_size == 5);
	assert(file.parameters.capacity == 10);
	ASSERT_FILE(
			16, 17, NIL, 42, 700,
			NIL, 900, 942, NIL, 1142);

	DELETE_AT(4);  // deletes 700
	DELETE_AT(6);  // deletes 900
	DELETE_AT(1);  // deletes 16
	ASSERT_FILE(
			NIL, NIL, 17, NIL, 42,
			NIL, NIL, 942, NIL, 1142);

	DELETE_AT(2);  // deletes 17
	assert(file.parameters.block_size == 4);
	assert(file.parameters.capacity == 4);
	ASSERT_FILE(42, 942, NIL, 1142);

	destroy_file(file);
}

static void test_resizing_through_trivial_cases() {
	struct ordered_file file;
	make_file(&file, 4,
			NIL, NIL, NIL, NIL);
	INSERT_FIRST(200);
	INSERT_AFTER(300, 0);
	INSERT_AFTER(400, 1);
	INSERT_FIRST(100);
	ASSERT_FILE(100, 200, 300, 400);

	INSERT_AFTER(500, 3);
	assert(file.parameters.block_size == 6);
	assert(file.parameters.capacity == 6);
	ASSERT_FILE(100, 200, 300, 400, 500, NIL);

	INSERT_AFTER(250, 1);
	assert(file.parameters.block_size == 4);
	assert(file.parameters.capacity == 8);
	ASSERT_FILE(
			100, 200, 250, 300,
			NIL, 400, 500, NIL);

	DELETE_AT(6);  // delete 500
	DELETE_AT(5);  // delete 400
	ASSERT_FILE(
			NIL, 100, NIL, 200,
			NIL, 250, NIL, 300);

	DELETE_AT(7);  // delete 300
	DELETE_AT(1);  // delete 100
	DELETE_AT(3);  // delete 200
	ASSERT_FILE(250, NIL, NIL, NIL);

	DELETE_AT(0);  // delete 250
	ASSERT_FILE(NIL, NIL, NIL, NIL);

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

	test_resizing_through_trivial_cases();
	test_comprehensive_resizing();
}
