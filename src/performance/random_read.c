#include "../test_hash/toycrypt.h"
#include "../log/log.h"
#include "../hash/hash.h"
#include "../stopwatch/stopwatch.h"
#include "../measurement/measurement.h"
#include "../rand/rand.h"

#include <inttypes.h>
#include <assert.h>

static uint64_t make_key(uint64_t i) {
	return toycrypt(i, 0x0123456789ABCDEFLL);
}

static uint64_t make_value(uint64_t i) {
	return toycrypt(i, 0xFEDCBA9876543210LL);
}

void time_random_reads(const hash_api* api, int size, int reads) {
	hash* table;
	if (hash_init(&table, api, NULL)) log_fatal("cannot init hash table");

	printf("random_reads: %s, size=%d, reads=%d...\n", api->name, size, reads);
	fflush(stdout);

	for (int i = 0; i < size; i++) {
		if (hash_insert(table, make_key(i), make_value(i))) log_fatal("cannot insert");
	}

	rand_generator generator = { .state = 0 };
	stopwatch watch = stopwatch_start();

	struct measurement_results results;
	{
		struct measurement measurement = measurement_begin();

		// Let every read be a hit.
		for (int i = 0; i < reads; i++) {
			int k = rand_next(&generator, size);
			uint64_t value;
			bool found;
			assert(!hash_find(table, make_key(k), &value, &found));
			assert(found && value == make_value(k));
		}

		results = measurement_end(measurement);
	}

	// hash_dump(table);

	uint64_t duration_ns = stopwatch_read_ns(watch);

	printf("%.2lf s (%" PRIu64 " ns/read)\n",
		((double) duration_ns) / (1000*1000*1000), duration_ns / reads);

	printf("\t%" PRIu64 " cache refs, %" PRIu64 " cache misses (%.2lf per read)\n",
		results.cache_references, results.cache_misses,
		((double) results.cache_misses) / reads);

	printf("\t");

	//hash_dump(table);

	hash_destroy(&table);
}