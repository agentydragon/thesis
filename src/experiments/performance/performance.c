#include <argp.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "cobt/cobt.h"
#include "dict/btree.h"
#include "dict/cobt.h"
#include "dict/dict.h"
#include "dict/splay.h"
#include "log/log.h"
#include "measurement/measurement.h"
#include "performance/random_read.h"
#include "rand/rand.h"
#include "stopwatch/stopwatch.h"
#include "util/human.h"

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

struct results {
	struct metrics combined, just_find;
};

struct results measure_api(const dict_api* api, uint64_t size) {
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

	struct results tr = {
		.combined = {
			.cache_misses = results_combined.cache_misses,
			.cache_references = results_combined.cache_references,
			.time_nsec = stopwatch_read_ns(watch)
		},
		.just_find = {
			.cache_misses = results_just_find.cache_misses,
			.cache_references = results_just_find.cache_references,
			.time_nsec = stopwatch_read_ns(watch_just_find)
		},
	};
	return tr;
}

// Those break.
struct {
	uint64_t maximum;
	double base;
} FLAGS;

static int parse_option(int key, char *arg, struct argp_state *state) {
	(void) arg; (void) state;
	switch (key) {
	case 'n':
		FLAGS.maximum = parse_human_i(arg);
		break;
	case 'b':
		FLAGS.base = parse_human_d(arg);
		break;
	case ARGP_KEY_ARG:
		log_fatal("unexpected argument: %s", arg);
	}
	return 0;
}

static void parse_flags(int argc, char** argv) {
	FLAGS.maximum = 64 * 1024 * 1024;
	FLAGS.base = 1.2;

	struct argp_option options[] = {
		{
			.name = NULL, .key = 'n', .arg = "MAX", .flags = 0,
			.doc = "Store MAX elements to store at most", .group = 0
		}, {
			.name = NULL, .key = 'b', .arg = "BASE", .flags = 0,
			.doc = "Multiply by BASE at each iteration", .group = 0
		}, { 0 }
	};
	struct argp argp = {
		.options = options, .parser = parse_option,
		.args_doc = NULL, .doc = NULL, .children = NULL,
		.help_filter = NULL, .argp_domain = NULL
	};
	assert(!argp_parse(&argp, argc, argv, 0, 0, 0));
}

int main(int argc, char** argv) {
	parse_flags(argc, argv);

	FILE* output = fopen("experiments/performance/results.tsv", "w");

	// TODO: merge with //performance.c
	for (double x = 10; x < FLAGS.maximum; x *= FLAGS.base) {
		COB_COUNTERS.total_reorganized_size = 0;

		const uint64_t size = round(x);
		log_info("size=%" PRIu64, size);
		struct results btree = measure_api(&dict_btree, size);
		struct results cobt = measure_api(&dict_cobt, size);
		struct results splay = measure_api(&dict_splay, size);

		fprintf(output,
				"%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
				"%" PRIu64
				"\n",
				size,
				btree.combined.cache_misses, btree.combined.cache_references, btree.combined.time_nsec,
				cobt.combined.cache_misses, cobt.combined.cache_references, cobt.combined.time_nsec,
				splay.combined.cache_misses, splay.combined.cache_references, splay.combined.time_nsec,

				btree.just_find.cache_misses, btree.just_find.cache_references, btree.just_find.time_nsec,
				cobt.just_find.cache_misses, cobt.just_find.cache_references, cobt.just_find.time_nsec,
				splay.just_find.cache_misses, splay.just_find.cache_references, splay.just_find.time_nsec,

				COB_COUNTERS.total_reorganized_size);
		fflush(output);
	}
	fclose(output);
	return 0;
}
