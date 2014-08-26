#include "test_hash.h"
#include "../log/log.h"

#include <stdlib.h>

static bool do_debug = false;

#define debug(fmt,...) do { \
	if (do_debug) { \
		printf(fmt "\n", ##__VA_ARGS__); \
	} \
} while (0)

static void has_element(hash* table, uint64_t key, uint64_t value) {
	uint64_t value_found;
	bool found;

	debug("has_element(%ld,%ld)", key, value);

	if (hash_find(table, key, &value_found, &found))
		log_fatal("hash_find for %ld failed", key);
	if (!found)
		log_fatal("key %ld not found", key);
	if (value_found != value)
		log_fatal("value for key %ld is %ld, not %ld", key, value, value_found);
}

static void has_no_element(hash* table, uint64_t key) {
	uint64_t value_found;
	bool found;

	debug("has_no_element(%ld)", key);

	if (hash_find(table, key, &value_found, &found))
		log_fatal("hash_find for %ld failed", key);
	if (found)
		log_fatal("key %ld found, expected not to find it", key);
}

static void insert(hash* table, uint64_t key, uint64_t value) {
	debug("insert(%ld,%ld)", key, value);

	if (hash_insert(table, key, value))
		log_fatal("cannot insert %ld=%ld to hash", key, value);
}

static void delete(hash* table, uint64_t key) {
	debug("delete(%ld)", key);

	if (hash_delete(table, key))
		log_fatal("cannot delete key %ld", key);
}

static void stores_two_elements(const hash_api* api) {
	hash* table;
	if (hash_init(&table, api)) log_fatal("cannot init hash table");

	debug("stores_two_elements(%s)", api->name);

	insert(table, 1, 10);
	insert(table, 2, 20);
	has_element(table, 1, 10);
	has_element(table, 2, 20);

	delete(table, 1);
	has_no_element(table, 1);
	has_element(table, 2, 20);

	delete(table, 2);
	has_no_element(table, 1);
	has_no_element(table, 2);

	insert(table, 1, 100);
	insert(table, 2, 200);
	has_element(table, 1, 100);
	has_element(table, 2, 200);

	delete(table, 2);
	has_element(table, 1, 100);
	has_no_element(table, 2);

	hash_destroy(&table);
}

static void stores_elements(const hash_api* api, uint64_t N) {
	hash* table;
	if (hash_init(&table, api)) log_fatal("cannot init hash table");

	debug("stores_elements(%s, %ld)", api->name, N);

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

#define it(behavior) do { \
	printf("  %40s ", #behavior); \
	behavior; \
	printf("✘\n"); \
} while (0)


void test_hash(const hash_api* api) {
	log_plain("%s", api->name);
	it(stores_two_elements(api));
	it(stores_elements(api, 10));
	it(stores_elements(api, 100));
	it(stores_elements(api, 1000));
	it(stores_elements(api, 2000));
}
