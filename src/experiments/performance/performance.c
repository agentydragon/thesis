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
#include "rand/rand.h"

static uint64_t make_key(uint64_t i) {
	return toycrypt(i, 0x0123456789ABCDEFLL);
}

static uint64_t make_value(uint64_t i) {
	return toycrypt(i, 0xFEDCBA9876543210LL);
}

struct metrics {
	measurement_results* results;
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

	measurement* measurement_just_find = measurement_begin();
	stopwatch watch_just_find = stopwatch_start();

	rand_generator generator = { .state = 0 };
	for (uint64_t i = 0; i < size; i++) {
		const int k = rand_next(&generator, used_ws_size);
		check_contains(table, make_key(k), make_value(k));
	}

	measurement_results* results_just_find =
			measurement_end(measurement_just_find);
	dict_destroy(&table);

	return (struct metrics) {
		.results = results_just_find,
		.time_nsec = stopwatch_read_ns(watch_just_find)
	};
}

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

		uint64_t next_key;
		assert(!dict_next(dict, current_key, &next_key, &found));
		if (found) {
			assert(next_key > current_key);
		}
		current_key = next_key;
	}
}

// TODO: measure_ltr_scan breaks the hacky implementation of splay_tree
struct metrics measure_ltr_scan(const dict_api* api, uint64_t size) {
	dict* table = seed(api, size);

	const int K = 3;  // for cache warmup
	for (int i = 0; i < K; i++) {
		iterate_ltr(table);
	}

	measurement* measurement_just_find = measurement_begin();
	stopwatch watch_just_find = stopwatch_start();
	iterate_ltr(table);
	measurement_results* results_just_find = measurement_end(measurement_just_find);
	dict_destroy(&table);

	return (struct metrics) {
		.results = results_just_find,
		.time_nsec = stopwatch_read_ns(watch_just_find)
	};
}

enum { SERIAL_BOTH, SERIAL_JUST_FIND } SERIAL_MODE = SERIAL_BOTH;
struct metrics measure_serial(const dict_api* api, uint64_t size) {
	measurement* measurement_both = measurement_begin();
	stopwatch watch = stopwatch_start();

	dict* table = seed(api, size);

	measurement* measurement_just_find = measurement_begin();
	stopwatch watch_just_find = stopwatch_start();

	rand_generator generator = { .state = 0 };
	// TODO: separate size/reads
	for (uint64_t i = 0; i < size; i++) {
		const int k = rand_next(&generator, size);
		// Let every read be a hit.
		check_contains(table, make_key(k), make_value(k));
	}

	measurement_results *results_combined = measurement_end(measurement_both),
			*results_just_find = measurement_end(measurement_just_find);
	dict_destroy(&table);

	switch (SERIAL_MODE) {
	case SERIAL_BOTH:
		measurement_results_release(results_just_find);
		return (struct metrics) {
			.results = results_combined,
			.time_nsec = stopwatch_read_ns(watch)
		};
	case SERIAL_JUST_FIND:
		measurement_results_release(results_combined);
		return (struct metrics) {
			.results = results_just_find,
			.time_nsec = stopwatch_read_ns(watch_just_find)
		};
	default:
		log_fatal("internal error");
	}
}

static void add_common_keys(json_t* point, const char* experiment,
		int size, const dict_api* api, struct metrics result) {
	json_object_set_new(point, "experiment", json_string(experiment));
	json_object_set_new(point, "implementation", json_string(api->name));
	json_object_set_new(point, "size", json_integer(size));
	json_object_set_new(point, "metrics",
			measurement_results_to_json(result.results));
	json_object_set_new(point, "time_ns", json_integer(result.time_nsec));
}

int main(int argc, char** argv) {
	parse_flags(argc, argv);

	// TODO: merge with //performance.c
	json_t* json_results = json_array();
	for (double x = 1; x < FLAGS.maximum; x *= FLAGS.base) {
		const uint64_t size = round(x);
		log_info("size=%" PRIu64, size);

		struct metrics result;
		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			SERIAL_MODE = SERIAL_BOTH;
			PMA_COUNTERS.reorganized_size = 0;
			result = measure_serial(FLAGS.measured_apis[i], size);

			json_t* point = json_object();
			add_common_keys(point, "serial-both", size,
					FLAGS.measured_apis[i], result);
			if (FLAGS.measured_apis[i] == &dict_cobt) {
				json_object_set_new(point, "pma_reorganized",
						json_integer(PMA_COUNTERS.reorganized_size));
			}
			json_array_append_new(json_results, point);
			measurement_results_release(result.results);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			SERIAL_MODE = SERIAL_JUST_FIND;
			result = measure_serial(FLAGS.measured_apis[i], size);

			json_t* point = json_object();
			add_common_keys(point, "serial-findonly", size,
					FLAGS.measured_apis[i], result);
			json_array_append_new(json_results, point);
			measurement_results_release(result.results);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			result = measure_working_set(FLAGS.measured_apis[i], size, 1000);

			json_t* point = json_object();
			add_common_keys(point, "workingset", size,
					FLAGS.measured_apis[i], result);
			json_object_set_new(point, "working_set_size", json_integer(1000));
			json_array_append_new(json_results, point);
			measurement_results_release(result.results);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			result = measure_working_set(FLAGS.measured_apis[i], size, 100000);

			json_t* point = json_object();
			add_common_keys(point, "workingset", size,
					FLAGS.measured_apis[i], result);
			json_object_set_new(point, "working_set_size", json_integer(100000));
			json_array_append_new(json_results, point);
			measurement_results_release(result.results);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			if (!FLAGS.measured_apis[i]->next) {
				// No implementation of ordering.
				// TODO: Make the test more idiomatic.
				continue;
			}
			result = measure_ltr_scan(FLAGS.measured_apis[i], size);

			json_t* point = json_object();
			add_common_keys(point, "ltr_scan", size,
					FLAGS.measured_apis[i], result);
			json_array_append_new(json_results, point);
			measurement_results_release(result.results);
		}

		log_info("flushing results...");
		assert(!json_dump_file(json_results,
					"experiments/performance/results.json",
					JSON_INDENT(2)));
		log_info("done");
	}
	json_decref(json_results);
	return 0;
}
