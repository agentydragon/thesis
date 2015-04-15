#include "dict/test/ordered_dict_blackbox.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include "log/log.h"

#define NIL 0xDEADDEADDEADDEAD

static void assert_map(dict* instance, uint64_t key, uint64_t value) {
	bool _found;
	uint64_t _found_value;
	dict_find(instance, key, &_found_value, &_found);
	if (!_found) {
		log_fatal("expected to find %" PRIu64 "=%" PRIu64 ", "
				"but no such key found",
				key, value);
	}
	if (_found_value != value) {
		log_fatal("expected to find %" PRIu64 "=%" PRIu64 ", "
				"but found %" PRIu64 "=%" PRIu64,
				key, value, key, _found_value);
	}
}

static void assert_next_key(dict* instance, uint64_t key, uint64_t next_key) {
	bool found;
	uint64_t found_key;
	dict_next(instance, key, &found_key, &found);
	if (next_key != NIL) {
		CHECK(found, "no next key for %" PRIu64 ", expected "
					"%" PRIu64, key, next_key);
		ASSERT(next_key == found_key);
	} else {
		CHECK(!found, "found next key %" PRIu64 " for %" PRIu64 ", "
				"expecting none", next_key, key);
	}
}

static void assert_previous_key(dict* instance,
		uint64_t key, uint64_t previous_key) {
	bool found;
	uint64_t found_key;
	dict_prev(instance, key, &found_key, &found);
	if (previous_key != NIL) {
		CHECK(found, "no previous key for %" PRIu64 ", expected "
					"%" PRIu64, key, previous_key);
		ASSERT(previous_key == found_key);
	} else {
		ASSERT(!found);
	}
}

static void check_equivalence(dict* instance, uint64_t N,
		uint64_t *keys, uint64_t *values, bool *present) {
	// Check that instance state matches our expectation.
	bool has_previous = false;
	uint64_t previous;
	for (uint64_t i = 0; i < N; i++) {
		if (present[i]) {
			assert_map(instance, keys[i], values[i]);
		} else {
			bool found;
			uint64_t found_value;
			dict_find(instance, keys[i], &found_value, &found);
			CHECK(!found, "Expected not to find key %" PRIu64 ", "
					"but it has value %" PRIu64 ".",
					keys[i], found_value);
		}

		bool has_next = false;
		uint64_t next;
		for (uint64_t j = i + 1; j < N; j++) {
			if (present[j]) {
				has_next = true;
				next = j;
				break;
			}
		}

		assert_previous_key(instance, keys[i], has_previous ? keys[previous] : NIL);
		assert_next_key(instance, keys[i], has_next ? keys[next] : NIL);

		if (present[i]) { has_previous = true; previous = i; }
	}
}

static void test_with_maximum_size(const dict_api* api, uint64_t N) {
	dict* instance;
	ASSERT(!dict_init(&instance, api, NULL));

	srand(0);
	uint64_t *keys = calloc(N, sizeof(uint64_t));
	uint64_t *values = calloc(N, sizeof(uint64_t));
	bool *present = calloc(N, sizeof(bool));

	// Generate random keys.
	keys[0] = rand() % 1000;
	for (uint64_t i = 1; i < N; i++) {
		keys[i] = keys[i - 1] + 1 + rand() % 1000;
		present[i] = false;
	}

	uint64_t current_size = 0;
	for (uint64_t iteration = 0; iteration < 10000; iteration++) {
		// log_info("iteration=%" PRIu64, iteration);

		if (current_size > 0 && (current_size >= N ||
					rand() % 3 == 0)) {
			// Find a random key and delete it.
			for (uint64_t i = 0; ; i = (i + 1) % N) {
				if (present[i] && rand() % current_size == 0) {
					// log_info("delete %" PRIu64, keys[i]);
					ASSERT(!dict_delete(instance, keys[i]));
					present[i] = false;
					--current_size;
					break;
				}
			}
		} else {
			// Add a random key.
			for (uint64_t i = 0; ; i = (i + 1) % N) {
				if (!present[i] && rand() % (N - current_size) == 0) {
					values[i] = rand();
					ASSERT(!dict_insert(instance, keys[i], values[i]));
					// log_info("add %" PRIu64 "=%" PRIu64, keys[i], values[i]);
					present[i] = true;
					++current_size;
					break;
				}
			}
		}

		dict_check(instance);
		check_equivalence(instance, N, keys, values, present);
	}

	dict_destroy(&instance);

	free(keys);
	free(values);
	free(present);
}

void test_ordered_dict_blackbox(const dict_api* api) {
	test_with_maximum_size(api, 10);
	test_with_maximum_size(api, 100);
	test_with_maximum_size(api, 1000);
}
