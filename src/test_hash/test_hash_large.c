#include "toycrypt.h"
#include "test_hash_large.h"
#include "../log/log.h"

#include <stdlib.h>
#include <inttypes.h> // PRIu64

// sizeof(struct bucket) == 32 => ~ 500 000 000 === ~14 GB pameti

// odmitne naalokovat uz 4.0GB blok :(

// It was too slow in Valgrind.

static uint64_t make_key(uint64_t i) {
	return toycrypt(i, 0x0123456789ABCDEFLL);
}

static uint64_t make_value(uint64_t i) {
	return toycrypt(i, 0xFEDCBA9876543210LL);
}

void test_hash_large(const hash_api* api, uint64_t N) {
	log_info("Testing %s.", api->name);

	hash* table;
	if (hash_init(&table, api, NULL)) {
		log_fatal("cannot init hash table");
	}

	log_info("Inserting %" PRIu64 " elements into %s...", N, api->name);
	for (uint64_t i = 0; i < N; i++) {
		if (i % 100000 == 0) log_info("%" PRIu64, i);
		const uint64_t key = make_key(i), value = make_value(i);
		// log_info("%" PRIu64 " => %" PRIu64, key, value);
		if (hash_insert(table, key, value))
			log_fatal("cannot insert");
	}

	hash_destroy(&table);
}
