#include "cobt/pma_test.h"

// TODO: clean up

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log/log.h"
#include "math/math.h"
#include "cobt/pma.h"

#define COUNTOF(x) (sizeof(x) / sizeof(*(x)))
#define NIL 0xDEADDEADDEADDEAD
#define UNDEF 0xDEADBEEF

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

static void test_parameter_policy(void) {
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

#define ITEM(x) ((pma_item) { .key = x, .value = (x) * 2 })

/*
void test_reorganization_complexity() {
//	for (uint64_t size = 2; size < 1000000; size += size / 2) {
	for (uint64_t size = 2; size < 10000; size += size / 2) {
		pma_COUNTERS.reorganized_size = 0;
		pma file;
		pma_init(&file);
		uint64_t HEAD = file.capacity;
		for (uint64_t i = size; i > 0; i--) {
			pma_insert_before(&file, ITEM(i), HEAD, &HEAD, NULL);
		}
		pma_destroy(file);

		log_info("%" PRIu64 "\t%" PRIu64,
				size, PMA_COUNTERS.reorganized_size);
	}
}
*/

void test_functional(void) {
	pma file;
	memset(&file, 0, sizeof(file));
	pma_init(&file);

	// Fills file with (1000 => 0), (999 => 500), (998 => 1000), ...
	for (uint64_t i = 0; i < 1000; i++) {
		void* mock_value = (void*) (i * 500);
		pma_insert_before(&file, (1000 - i), mock_value, file.capacity);
	}
	uint64_t seen = 0;
	for (uint64_t i = 0; i < file.capacity; i++) {
		if (file.occupied[i]) {
			CHECK(file.keys[i] == (1000 - seen),
					"expected key %" PRIu64 ", found "
					"%" PRIu64 ".", seen, file.keys[i]);
			assert(file.values[i] == (void*) (seen * 500));
			++seen;
		}
	}
	assert(seen == 1000);

	pma_destroy(&file);
}

void test_cobt_pma(void) {
	test_functional();
//	test_parameter_policy();
//	test_reorganization_complexity();
}
