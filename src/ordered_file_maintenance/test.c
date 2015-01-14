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

/*
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
#define ASSERT_FILE(...) assert_file(file, __VA_ARGS__)

*/

/*
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
*/

#define ITEM(x) ((ofm_item) { .key = x, .value = (x) * 2 })

void test_reorganization_complexity() {
	for (uint64_t size = 2; size < 1000000; size += size / 2) {
		OFM_COUNTERS.reorganized_size = 0;
		ofm file;
		ofm_init(&file);
		uint64_t HEAD = file.capacity;
		for (uint64_t i = size; i > 0; i--) {
			ofm_insert_before(&file, ITEM(i), HEAD, &HEAD, NULL);
		}
		ofm_destroy(file);

		log_info("%" PRIu64 "\t%" PRIu64,
				size, OFM_COUNTERS.reorganized_size);
	}
}

/*
void test_functional() {
	ofm file;
	ofm_init(&file);

	uint64_t HEAD = file.capacity;
	for (uint64_t i = 4000; i > 0; i--) {
		ofm_insert_before(&file, ITEM(i), HEAD, &HEAD, NULL);
		ofm_dump(file);
	}

	ofm_destroy(file);
}
*/

void test_ordered_file_maintenance() {
//	test_parameter_policy();
//	test_functional();
	test_reorganization_complexity();
}
