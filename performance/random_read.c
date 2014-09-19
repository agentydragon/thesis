#include "../test_hash/toycrypt.h"
#include "../log/log.h"
#include "../hash/hash.h"
#include "../stopwatch/stopwatch.h"
#include "../measurement/measurement.h"

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

	srand(0);
	stopwatch watch = stopwatch_start();

	struct measurement_results results;
	{
		struct measurement measurement = measurement_begin();

		// Let every read be a hit.
		for (int i = 0; i < reads; i++) {
			int k = rand() % size;
			uint64_t value;
			bool found;
			if (hash_find(table, make_key(k), &value, &found)) log_fatal("cannot insert");
			assert(found && value == make_value(k));
		}

		results = measurement_end(measurement);
	}

	uint64_t duration = stopwatch_read_nsec(watch);
	printf("%" PRIu64 " nsec (%" PRIu64 " nsec/read, %" PRIu64 " cache references, "
		"%" PRIu64 " cache misses, %.2lf cache misses/read)\n",
		duration, duration / reads,
		results.cache_references, results.cache_misses,
		((double) results.cache_misses) / reads);

	hash_destroy(&table);
}
