#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "cobt/cobt.h"
#include "experiments/performance/flags.h"
#include "log/log.h"
#include "measurement/measurement.h"
#include "performance/random_read.h"
#include "rand/rand.h"
#include "stopwatch/stopwatch.h"

static uint64_t make_key(uint64_t i) {
	return toycrypt(i, 0x0123456789ABCDEFLL);
}

static uint64_t make_value(uint64_t i) {
	return toycrypt(i, 0xFEDCBA9876543210LL);
}

struct metrics {
	uint64_t cache_misses;
	uint64_t cache_references;
	uint64_t time_nsec;
};

static void insert(dict* dict, uint64_t key, uint64_t value) {
	CHECK(key != RESERVED_KEY, "trying to insert reserved key");
	CHECK(!dict_insert(table, key, value), "cannot insert");
}

struct metrics measure_working_set(const dict_api* api, uint64_t size,
		uint64_t working_set_size) {
	struct measurement measurement = measurement_begin();
	stopwatch watch = stopwatch_start();

	const uint64_t used_ws_size = working_set_size < size ? working_set_size : size;

	dict* table;
	CHECK(!dict_init(&table, api, NULL), "cannot init dict");
	for (uint64_t i = 0; i < size; i++) {
		insert(table, make_key(i), make_value(i));
	}

	struct measurement measurement_just_find = measurement_begin();
	stopwatch watch_just_find = stopwatch_start();

	rand_generator generator = { .state = 0 };
	for (uint64_t i = 0; i < size; i++) {
		int k = rand_next(&generator, used_ws_size);
		uint64_t value;
		bool found;
		assert(!dict_find(table, make_key(k), &value, &found));
		assert(found && value == make_value(k));
	}

	struct measurement_results results_combined = measurement_end(measurement),
				   results_just_find = measurement_end(measurement_just_find);
	dict_destroy(&table);

	return (struct metrics) {
		.cache_misses = results_just_find.cache_misses,
		.cache_references = results_just_find.cache_references,
		.time_nsec = stopwatch_read_ns(watch_just_find)
	};
}

enum { SERIAL_BOTH, SERIAL_JUST_FIND } SERIAL_MODE = SERIAL_BOTH;
struct metrics measure_serial(const dict_api* api, uint64_t size) {
	struct measurement measurement = measurement_begin();
	stopwatch watch = stopwatch_start();

	dict* table;
	if (dict_init(&table, api, NULL)) log_fatal("cannot init dict");
	for (uint64_t i = 0; i < size; i++) {
		if (dict_insert(table, make_key(i), make_value(i))) {
			log_fatal("cannot insert");
		}
	}

	struct measurement measurement_just_find = measurement_begin();
	stopwatch watch_just_find = stopwatch_start();

	rand_generator generator = { .state = 0 };
	// Let every read be a hit.
	// TODO: separate size/reads
	for (uint64_t i = 0; i < size; i++) {
		int k = rand_next(&generator, size);
		uint64_t value;
		bool found;
		assert(!dict_find(table, make_key(k), &value, &found));
		assert(found && value == make_value(k));
	}

	struct measurement_results results_combined = measurement_end(measurement),
				   results_just_find = measurement_end(measurement_just_find);
	dict_destroy(&table);

	switch (SERIAL_MODE) {
	case SERIAL_BOTH:
		return (struct metrics) {
			.cache_misses = results_combined.cache_misses,
			.cache_references = results_combined.cache_references,
			.time_nsec = stopwatch_read_ns(watch)
		};
	case SERIAL_JUST_FIND:
		return (struct metrics) {
			.cache_misses = results_just_find.cache_misses,
			.cache_references = results_just_find.cache_references,
			.time_nsec = stopwatch_read_ns(watch_just_find)
		};
	default:
		log_fatal("internal error");
	}
}

int main(int argc, char** argv) {
	parse_flags(argc, argv);

	FILE* output = fopen("experiments/performance/results.tsv", "w");

	// TODO: merge with //performance.c
	for (double x = 10; x < FLAGS.maximum; x *= FLAGS.base) {
		COB_COUNTERS.total_reorganized_size = 0;

		const uint64_t size = round(x);
		log_info("size=%" PRIu64, size);

		fprintf(output, "%" PRIu64 "\t", size);

		struct metrics results[10];
		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			SERIAL_MODE = SERIAL_BOTH;
			results[i] = measure_serial(FLAGS.measured_apis[i], size);
			fprintf(output, "%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t",
					results[i].cache_misses,
					results[i].cache_references,
					results[i].time_nsec);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			SERIAL_MODE = SERIAL_JUST_FIND;
			results[i] = measure_serial(FLAGS.measured_apis[i], size);
			fprintf(output, "%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t",
					results[i].cache_misses,
					results[i].cache_references,
					results[i].time_nsec);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			results[i] = measure_working_set(FLAGS.measured_apis[i], size, 1000);
			fprintf(output, "%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t",
					results[i].cache_misses,
					results[i].cache_references,
					results[i].time_nsec);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			results[i] = measure_working_set(FLAGS.measured_apis[i], size, 100000);
			fprintf(output, "%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t",
					results[i].cache_misses,
					results[i].cache_references,
					results[i].time_nsec);
		}

		fprintf(output, "%" PRIu64 "\n",
				COB_COUNTERS.total_reorganized_size);
		fflush(output);
	}
	fclose(output);
	return 0;
}
