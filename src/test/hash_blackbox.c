#include "hash_blackbox.h"
#include "../log/log.h"
#include "../stopwatch/stopwatch.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static void has_element(hash* table, uint64_t key, uint64_t value) {
	uint64_t value_found;
	bool found;

	//log_info("has_element(%ld,%ld)", key, value);

	CHECK(hash_find(table, key, &value_found, &found) == 0,
			"hash_find for %ld failed", key);
	CHECK(found, "key %ld not found", key);
	CHECK(value_found == value,
			"value for key %ld is %ld, not %ld",
			key, value, value_found);
}

static void has_no_element(hash* table, uint64_t key) {
	uint64_t value_found;
	bool found;

	//log_info("has_no_element(%ld)", key);

	if (hash_find(table, key, &value_found, &found)) {
		log_fatal("hash_find for %ld failed", key);
	}
	if (found) {
		log_fatal("key %ld found, expected not to find it", key);
	}
}

static void insert(hash* table, uint64_t key, uint64_t value) {
	//log_info("insert(%ld,%ld)", key, value);

	if (hash_insert(table, key, value)) {
		log_fatal("cannot insert %ld=%ld to hash", key, value);
	}
}

static void delete(hash* table, uint64_t key) {
	//log_info("delete(%ld)", key);

	if (hash_delete(table, key)) {
		log_fatal("cannot delete key %ld", key);
	}
}

static void has_no_elements_at_first(const hash_api* api) {
	hash* table;
	if (hash_init(&table, api, NULL)) {
		log_fatal("cannot init hash table");
	}

	has_no_element(table, 1);
	has_no_element(table, 2);

	hash_destroy(&table);
}

static void doesnt_delete_at_first(const hash_api* api) {
	hash* table;
	if (hash_init(&table, api, NULL)) {
		log_fatal("cannot init hash table");
	}

	assert(hash_delete(table, 1));
	assert(hash_delete(table, 2));

	hash_destroy(&table);
}

static void check_equivalence(hash* instance, uint64_t N,
		uint64_t* keys, uint64_t* values, bool* present) {
	// TODO: randomize test order?
	for (uint64_t i = 0; i < N; i++) {
		uint64_t value;
		bool key_present;
		assert(!hash_find(instance, keys[i], &value, &key_present));
		if (present[i]) {
			CHECK(key_present, "expected to find %" PRIu64 "=%" PRIu64 ", "
						"but no such key found",
						keys[i], values[i]);
		} else {
			assert(!key_present);
		}
		if (key_present) {
			assert(value == values[i]);
		}
	}
}

static void test_with_maximum_size(const hash_api* api, uint64_t N) {
	//log_info("===");
	hash* instance;
	if (hash_init(&instance, api, NULL)) {
		log_fatal("cannot init hash table");
	}

	srand(0);
	uint64_t *keys = calloc(N, sizeof(uint64_t));
	uint64_t *values = calloc(N, sizeof(uint64_t));
	bool *present = calloc(N, sizeof(bool));

	keys[0] = rand() % 1000;
	for (uint64_t i = 1; i < N; i++) {
		keys[i] = keys[i - 1] + 1 + rand() % 1000;
		present[i] = false;
	}

	uint64_t current_size = 0;
	for (uint64_t iteration = 0; iteration < 100000; iteration++) {
		if (current_size > 0 && (current_size >= N ||
					rand() % 3 == 0)) {
			// Find a random key and delete it.
			for (uint64_t i = 0; ; i = (i + 1) % N) {
				if (present[i] && rand() % current_size == 0) {
					delete(instance, keys[i]);
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
					insert(instance, keys[i], values[i]);
					present[i] = true;
					++current_size;
					break;
				}
			}
		}

		check_equivalence(instance, N, keys, values, present);
	}

	hash_destroy(&instance);

	free(keys);
	free(values);
	free(present);
}

static void test_regular(const hash_api* api, uint64_t N) {
	hash* table;
	CHECK(hash_init(&table, api, NULL) == 0, "cannot init hash table");

	log_info("test_regular(%s, %ld)", api->name, N);

	for (uint64_t i = 0; i < N; i++) {
		insert(table, i * 3, i * 7);

		for (uint64_t j = 0; j < N; j++) {
			if (j <= i) {
				has_element(table, j * 3, j * 7);
			} else {
				has_no_element(table, j * 3);
			}
		}
	}

	hash_destroy(&table);
}

void test_hash_blackbox(const hash_api* api) {
	log_info("performing blackbox test on %s", api->name);
	has_no_elements_at_first(api);
	doesnt_delete_at_first(api);

	//stores_two_elements(api);

	test_regular(api, 10);
	test_regular(api, 20);
	test_regular(api, 50);
	test_regular(api, 100);
	test_regular(api, 200);
	// There was a 500, 1000 and 2000 test here, but they were slow.

	test_with_maximum_size(api, 10);
	test_with_maximum_size(api, 20);
	test_with_maximum_size(api, 50);
	test_with_maximum_size(api, 100);
	test_with_maximum_size(api, 200);

	// TODO: more complex deletion tests.
	// TODO: find accepts NULLs
}
