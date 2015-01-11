#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "../../hash/hash.h"
#include "../../hash_bplustree/hash_bplustree.h"
#include "../../hash_cobt/hash_cobt.h"
#include "../../performance/random_read.h"
#include "../../measurement/measurement.h"
#include "../../stopwatch/stopwatch.h"

#include "../../log/log.h"
#include "../../rand/rand.h"

static uint64_t make_key(uint64_t i) {
	return toycrypt(i, 0x0123456789ABCDEFLL);
}

static uint64_t make_value(uint64_t i) {
	return toycrypt(i, 0xFEDCBA9876543210LL);
}

struct results {
	uint64_t cache_misses;
	uint64_t cache_references;
	uint64_t time_nsec;
};

struct results measure_api(const hash_api* api, uint64_t size) {
	struct measurement measurement = measurement_begin();

	stopwatch watch = stopwatch_start();

	hash* table;
	if (hash_init(&table, api, NULL)) log_fatal("cannot init hash table");
	for (int i = 0; i < size; i++) {
		if (hash_insert(table, make_key(i), make_value(i))) {
			log_fatal("cannot insert");
		}
	}

	rand_generator generator = { .state = 0 };
	// Let every read be a hit.
	// TODO: separate size/reads
	for (int i = 0; i < size; i++) {
		int k = rand_next(&generator, size);
		uint64_t value;
		bool found;
		assert(!hash_find(table, make_key(k), &value, &found));
		assert(found && value == make_value(k));
	}

	struct measurement_results results = measurement_end(measurement);

	hash_destroy(&table);

	struct results tr = {
		.cache_misses = results.cache_misses,
		.cache_references = results.cache_references,
		.time_nsec = stopwatch_read_ns(watch)
	};
	return tr;
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;
	FILE* output = fopen("experiments/cob_cache_behavior/results.csv", "w");
	double base = 1.2;
	double x = 1000;

	while (x < 512 * 1024) {
		const uint64_t size = round(x);
		log_info("size=%" PRIu64, size);
		struct results bplustree = measure_api(&hash_bplustree, size);
		struct results cobt = measure_api(&hash_cobt, size);

		fprintf(output,
				"%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64
				"\n",
				size,
				bplustree.cache_misses, bplustree.cache_references, bplustree.time_nsec,
				cobt.cache_misses, cobt.cache_references, cobt.time_nsec);
		x *= base;
	}
	fclose(output);
	return 0;
}
