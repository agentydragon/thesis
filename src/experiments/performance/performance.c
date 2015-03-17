#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "cobt/cobt.h"
#include "dict/cobt.h"
#include "dict/test/toycrypt.h"
#include "experiments/performance/flags.h"
#include "log/log.h"
#include "measurement/measurement.h"
#include "measurement/stopwatch.h"
#include "performance/random_read.h"
#include "rand/rand.h"

static uint64_t make_key(uint64_t i) {
	return toycrypt(i, 0x0123456789ABCDEFLL);
}

static uint64_t make_value(uint64_t i) {
	return toycrypt(i, 0xFEDCBA9876543210LL);
}

struct metrics {
	struct measurement_results measurement_results;
	uint64_t time_nsec;
};

static void insert(dict* dict, uint64_t key, uint64_t value) {
	CHECK(key != DICT_RESERVED_KEY, "trying to insert reserved key");
	CHECK(!dict_insert(dict, key, value), "cannot insert");
}

static void check_contains(dict* dict, uint64_t key, uint64_t value) {
	uint64_t found_value;
	bool found;
	assert(!dict_find(dict, key, &found_value, &found));
	assert(found && found_value == value);
}

dict* seed(const dict_api* api, uint64_t size) {
	dict* dict;
	CHECK(!dict_init(&dict, api, NULL), "cannot init dict");
	for (uint64_t i = 0; i < size; i++) {
		insert(dict, make_key(i), make_value(i));
	}
	return dict;
}

struct metrics measure_working_set(const dict_api* api, uint64_t size,
		uint64_t working_set_size) {
	const uint64_t used_ws_size =
			working_set_size < size ? working_set_size : size;

	dict* table = seed(api, size);

	struct measurement measurement_just_find = measurement_begin();
	stopwatch watch_just_find = stopwatch_start();

	rand_generator generator = { .state = 0 };
	for (uint64_t i = 0; i < size; i++) {
		const int k = rand_next(&generator, used_ws_size);
		check_contains(table, make_key(k), make_value(k));
	}

	struct measurement_results results_just_find =
			measurement_end(measurement_just_find);
	dict_destroy(&table);

	return (struct metrics) {
		.measurement_results = results_just_find,
		.time_nsec = stopwatch_read_ns(watch_just_find)
	};
}

/*
static void iterate_ltr(dict* dict) {
	uint64_t min = 0;
	bool found;
	assert(!dict_find(dict, min, NULL, &found));
	if (!found) {
		assert(!dict_next(dict, min, &min, &found));
	}

	uint64_t current_key = min;
	while (found) {
		uint64_t value;
		assert(!dict_find(dict, current_key, &value, &found));
		(void) value;  // unused
		assert(found);

		assert(!dict_next(dict, current_key, &current_key, &found));
	}
}
*/

// TODO: measure_ltr_scan breaks the hacky implementation of splay_tree
/*
struct metrics measure_ltr_scan(const dict_api* api, uint64_t size) {
	dict* table = seed(api, size);

	struct measurement measurement_just_find = measurement_begin();
	stopwatch watch_just_find = stopwatch_start();

	const int K = 5;  // for cache warmup
	for (int i = 0; i < K; i++) {
		iterate_ltr(table);
	}

	struct measurement_results results_just_find = measurement_end(measurement_just_find);
	dict_destroy(&table);

	return (struct metrics) {
		.measurement_results = {
			.cache_misses = results_just_find.cache_misses / K,
			.cache_references = results_just_find.cache_references / K
		},
		.time_nsec = stopwatch_read_ns(watch_just_find) / K
	};
}
*/

enum { SERIAL_BOTH, SERIAL_JUST_FIND } SERIAL_MODE = SERIAL_BOTH;
struct metrics measure_serial(const dict_api* api, uint64_t size) {
	struct measurement measurement = measurement_begin();
	stopwatch watch = stopwatch_start();

	dict* table = seed(api, size);

	struct measurement measurement_just_find = measurement_begin();
	stopwatch watch_just_find = stopwatch_start();

	rand_generator generator = { .state = 0 };
	// TODO: separate size/reads
	for (uint64_t i = 0; i < size; i++) {
		const int k = rand_next(&generator, size);
		// Let every read be a hit.
		check_contains(table, make_key(k), make_value(k));
	}

	struct measurement_results results_combined = measurement_end(measurement),
				   results_just_find = measurement_end(measurement_just_find);
	dict_destroy(&table);

	switch (SERIAL_MODE) {
	case SERIAL_BOTH:
		return (struct metrics) {
			.measurement_results = results_combined,
			.time_nsec = stopwatch_read_ns(watch)
		};
	case SERIAL_JUST_FIND:
		return (struct metrics) {
			.measurement_results = results_just_find,
			.time_nsec = stopwatch_read_ns(watch_just_find)
		};
	default:
		log_fatal("internal error");
	}
}

static void add_common_keys(json_t* point,
		const dict_api* api, struct metrics result) {
	json_object_set_new(point, "implementation", json_string(api->name));
	json_object_set_new(point, "metrics",
			measurement_results_to_json(result.measurement_results));
	json_object_set_new(point, "time_ns", json_integer(result.time_nsec));
}

int main(int argc, char** argv) {
	parse_flags(argc, argv);

	// TODO: merge with //performance.c
	json_t* json_results = json_array();
	for (double x = 10; x < FLAGS.maximum; x *= FLAGS.base) {

		const uint64_t size = round(x);
		log_info("size=%" PRIu64, size);

		struct metrics result;
		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			SERIAL_MODE = SERIAL_BOTH;
			OFM_COUNTERS.reorganized_size = 0;
			result = measure_serial(FLAGS.measured_apis[i], size);

			json_t* point = json_object();
			add_common_keys(point, FLAGS.measured_apis[i], result);
			json_object_set_new(point, "experiment",
					json_string("serial-both"));
			json_object_set_new(point, "size", json_integer(x));
			if (FLAGS.measured_apis[i] == &dict_cobt) {
				json_object_set_new(point, "ofm_reorganized",
						json_integer(OFM_COUNTERS.reorganized_size));
			}
			json_array_append_new(json_results, point);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			SERIAL_MODE = SERIAL_JUST_FIND;
			result = measure_serial(FLAGS.measured_apis[i], size);

			json_t* point = json_object();
			add_common_keys(point, FLAGS.measured_apis[i], result);
			json_object_set_new(point, "experiment",
					json_string("serial-findonly"));
			json_object_set_new(point, "size", json_integer(x));
			json_array_append_new(json_results, point);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			result = measure_working_set(FLAGS.measured_apis[i], size, 1000);

			json_t* point = json_object();
			add_common_keys(point, FLAGS.measured_apis[i], result);
			json_object_set_new(point, "experiment", json_string("workingset"));
			json_object_set_new(point, "working_set_size", json_integer(1000));
			json_object_set_new(point, "size", json_integer(x));
			json_array_append_new(json_results, point);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			result = measure_working_set(FLAGS.measured_apis[i], size, 100000);

			json_t* point = json_object();
			add_common_keys(point, FLAGS.measured_apis[i], result);
			json_object_set_new(point, "experiment", json_string("workingset"));
			json_object_set_new(point, "working_set_size", json_integer(100000));
			json_object_set_new(point, "size", json_integer(x));
			json_array_append_new(json_results, point);
		}

		/*
		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			// "implements ordering"?
			if (FLAGS.measured_apis[i]->next) {
				results[i] = measure_ltr_scan(FLAGS.measured_apis[i], size);
			}
		}
		*/

		assert(!json_dump_file(json_results,
					"experiments/performance/results.json",
					JSON_INDENT(2)));
	}
	json_decref(json_results);
	return 0;
}
