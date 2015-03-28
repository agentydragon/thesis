#include "dict/test/blackbox.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "log/log.h"

static void has_element(dict* table, uint64_t key, uint64_t value) {
	uint64_t value_found;
	bool found;
	CHECK(dict_find(table, key, &value_found, &found) == 0,
			"dict_find for %ld failed", key);
	CHECK(found, "key %ld not found", key);
	CHECK(value_found == value,
			"value for key %ld should be %ld, but is %ld",
			key, value, value_found);
}

static void init(dict** table, const dict_api* api) {
	CHECK(!dict_init(table, api, NULL), "cannot init dict");
}

static void has_no_element(dict* table, uint64_t key) {
	bool found;
	CHECK(!dict_find(table, key, NULL, &found),
		"dict_find for %ld failed", key);
	CHECK(!found, "key %ld found, expected not to find it", key);
}

static void insert(dict* table, uint64_t key, uint64_t value) {
	log_verbose(1, "insert(%ld,%ld)", key, value);
	CHECK(!dict_insert(table, key, value), "cannot insert %ld=%ld to dict",
			key, value);
}

static void delete(dict* table, uint64_t key) {
	log_verbose(1, "delete(%ld)", key);
	CHECK(!dict_delete(table, key), "cannot delete key %ld", key);
}

static void has_no_elements_at_first(const dict_api* api) {
	dict* table;
	init(&table, api);

	has_no_element(table, 1);
	has_no_element(table, 2);

	dict_destroy(&table);
}

static void doesnt_delete_at_first(const dict_api* api) {
	dict* table;
	init(&table, api);

	ASSERT(dict_delete(table, 1));
	ASSERT(dict_delete(table, 2));

	dict_destroy(&table);
}

static void check_equivalence(dict* instance, uint64_t N,
		uint64_t* keys, uint64_t* values, bool* present) {
	// TODO: randomize test order?
	for (uint64_t i = 0; i < N; i++) {
		uint64_t value;
		bool key_present;
		ASSERT(!dict_find(instance, keys[i], &value, &key_present));
		if (present[i]) {
			CHECK(key_present, "expected to find %" PRIu64 "=%" PRIu64 ", "
						"but no such key found",
						keys[i], values[i]);
		} else {
			ASSERT(!key_present);
		}
		if (key_present) {
			ASSERT(value == values[i]);
		}
	}
}

static void test_with_maximum_size(const dict_api* api, uint64_t N) {
	dict* instance;
	init(&instance, api);

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

	dict_destroy(&instance);

	free(keys);
	free(values);
	free(present);
}

static void test_regular(const dict_api* api, uint64_t N) {
	dict* table;
	init(&table, api);

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

	dict_destroy(&table);
}

static void fails_on_duplicate_insertion(const dict_api* api) {
	dict* table;
	init(&table, api);
	insert(table, 10, 20);
	CHECK(dict_insert(table, 10, 20),
			"duplicate insertion should fail, but it succeeded");

	has_element(table, 10, 20);

	dict_destroy(&table);
}

void test_dict_blackbox(const dict_api* api) {
	log_info("testing %s", api->name);
	has_no_elements_at_first(api);
	doesnt_delete_at_first(api);
	fails_on_duplicate_insertion(api);

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
