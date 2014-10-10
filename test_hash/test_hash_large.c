#include "toycrypt.h"
#include "test_hash_large.h"
#include "../log/log.h"

#include <inttypes.h> // PRIu64

// sizeof(struct bucket) == 32 => ~ 500 000 000 === ~14 GB pameti

// odmitne naalokovat uz 4.0GB blok :(

// 2**25 zabira asi 1 GB pameti; je to radove 33 000 000.
// static uint64_t N = (1L << 25);

// It was too slow in Valgrind.

static uint64_t make_key(uint64_t i) {
	return toycrypt(i, 0x0123456789ABCDEFLL);
}

static uint64_t make_value(uint64_t i) {
	return toycrypt(i, 0xFEDCBA9876543210LL);
}

void test_hash_large(const hash_api* api, uint64_t N) {
	(void) api;
	/*
	for (uint64_t i = 0; i < 32; i++) {
		log_info("%3" PRIu64 " => %" PRIu64, i, toycrypt(i, key));
	}
	*/

	hash* table;
	if (hash_init(&table, api, NULL)) log_fatal("cannot init hash table");

	log_info("Inserting %" PRIu64 " elements...", N);
	for (uint64_t i = 0; i < N; i++) {
		if (i % 100000 == 0) log_info("%" PRIu64, i);
		if (hash_insert(table, make_key(i), make_value(i)))
			log_fatal("cannot insert");
	}

	log_info("Inserted");

	hash_destroy(&table);
}
