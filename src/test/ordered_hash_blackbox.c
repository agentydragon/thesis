#include "ordered_hash_blackbox.h"
#include "../log/log.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#define NIL 0xDEADDEADDEADDEAD

static void assert_map(
		ordered_hash_blackbox_spec api, void* instance,
		uint64_t key, uint64_t value) {
	bool _found;
	uint64_t _found_value;
	api.find(instance, key, &_found, &_found_value);
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

static void assert_next_key(
		ordered_hash_blackbox_spec api, void* instance,
		uint64_t key, uint64_t next_key) {
	bool found;
	uint64_t found_key;
	api.next_key(instance, key, &found, &found_key);
	if (next_key != NIL) {
		CHECK(found, "no next key for %" PRIu64 ", expected "
					"%" PRIu64, key, next_key);
		assert(next_key == found_key);
	} else {
		assert(!found);
	}
}

static void assert_previous_key(
		ordered_hash_blackbox_spec api, void* instance,
		uint64_t key, uint64_t previous_key) {
	bool found;
	uint64_t found_key;
	api.previous_key(instance, key, &found, &found_key);
	if (previous_key != NIL) {
		CHECK(found, "no previous key for %" PRIu64 ", expected "
					"%" PRIu64, key, previous_key);
		assert(previous_key == found_key);
	} else {
		assert(!found);
	}
}

static void check_equivalence(
		ordered_hash_blackbox_spec api, void* instance,
		uint64_t N, uint64_t *keys, uint64_t *values, bool *present) {
	// Check that instance state matches our expectation.
	bool has_previous = false;
	uint64_t previous;
	for (uint64_t i = 0; i < N; i++) {
		if (present[i]) {
			assert_map(api, instance, keys[i], values[i]);
		} else {
			bool found;
			api.find(instance, keys[i], &found, NULL);
			assert(!found);
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

		assert_previous_key(api, instance, keys[i], has_previous ? keys[previous] : NIL);
		assert_next_key(api, instance, keys[i], has_next ? keys[next] : NIL);

		if (present[i]) { has_previous = true; previous = i; }
	}
}

void test_ordered_hash_blackbox(ordered_hash_blackbox_spec api) {
	void* instance;
	api.init(&instance);

	srand(0);
	const uint64_t N = 100;
	uint64_t *keys = calloc(N, sizeof(uint64_t));
	uint64_t *values = calloc(N, sizeof(uint64_t));
	bool *present = calloc(N, sizeof(bool));

	// Generate random keys.
	keys[0] = rand() % 1000;
	for (uint64_t i = 1; i < N; i++) {
		keys[i] = keys[i - 1] + rand() % 1000;
		present[i] = false;
	}

	uint64_t current_size = 0;
	const uint64_t max_size = N;
	for (uint64_t iteration = 0; iteration < 1000; iteration++) {
		log_info("iteration=%" PRIu64, iteration);

		if (current_size > 0 && (current_size >= max_size ||
					rand() % 3 == 0)) {
			// Find a random key and delete it.
			for (uint64_t i = 0; ; i = (i + 1) % N) {
				if (present[i] && rand() % current_size == 0) {
					log_info("delete %" PRIu64, keys[i]);
					api.remove(instance, keys[i]);
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
					api.insert(instance, keys[i],values[i]);
					log_info("add %" PRIu64 "=%" PRIu64, keys[i], values[i]);
					present[i] = true;
					++current_size;
					break;
				}
			}
		}

		if (api.check != NULL) {
			api.check(instance);
		}
		check_equivalence(api, instance, N, keys, values, present);
	}

	api.destroy(&instance);

	free(keys);
	free(values);
	free(present);
}
