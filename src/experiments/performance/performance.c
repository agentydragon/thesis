#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "cobt/cobt.h"
#include "dict/cobt.h"
#include "dict/dict.h"
#include "dict/htcuckoo.h"
#include "dict/ksplay.h"
#include "experiments/performance/flags.h"
#include "experiments/performance/ltr_scan.h"
#include "experiments/performance/serial.h"
#include "experiments/performance/word_frequency.h"
#include "experiments/performance/working_set.h"
#include "htable/cuckoo.h"
#include "ksplay/ksplay.h"
#include "log/log.h"
#include "measurement/measurement.h"
#include "measurement/stopwatch.h"
#include "rand/rand.h"

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
	init_word_frequency();

	// TODO: merge with //performance.c
	json_t* json_results = json_array();
	for (double x = 1; x < FLAGS.maximum; x *= FLAGS.base) {
		const uint64_t size = round(x);
		log_info("size=%" PRIu64, size);

		struct metrics result;
		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			PMA_COUNTERS.reorganized_size = 0;
			CUCKOO_COUNTERS.inserts = 0;
			CUCKOO_COUNTERS.full_rehashes = 0;
			CUCKOO_COUNTERS.traversed_edges = 0;

			result = measure_serial(FLAGS.measured_apis[i],
					SERIAL_BOTH, size);

			json_t* point = json_object();
			add_common_keys(point, "serial-both", size,
					FLAGS.measured_apis[i], result);
			if (FLAGS.measured_apis[i] == &dict_cobt) {
				json_object_set_new(point, "pma_reorganized",
						json_integer(PMA_COUNTERS.reorganized_size));
			}
			if (FLAGS.measured_apis[i] == &dict_htcuckoo) {
				json_object_set_new(point, "cuckoo_inserts",
						json_integer(CUCKOO_COUNTERS.inserts));
				json_object_set_new(point, "cuckoo_full_rehashes",
						json_integer(CUCKOO_COUNTERS.full_rehashes));
				json_object_set_new(point, "cuckoo_traversed_edges",
						json_integer(CUCKOO_COUNTERS.traversed_edges));
			}
			json_array_append_new(json_results, point);
			measurement_results_release(result.results);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			result = measure_serial(FLAGS.measured_apis[i],
					SERIAL_JUST_INSERT, size);

			json_t* point = json_object();
			add_common_keys(point, "serial-insertonly", size,
					FLAGS.measured_apis[i], result);
			json_array_append_new(json_results, point);
			measurement_results_release(result.results);
		}
		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			result = measure_serial(FLAGS.measured_apis[i],
					SERIAL_JUST_FIND, size);

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
			const dict_api* api = FLAGS.measured_apis[i];
			if (!dict_api_allows_order_queries(api)) {
				log_info("order queries not supported by %s, "
						"skipping.", api->name);
				continue;
			}
			KSPLAY_COUNTERS.ksplay_steps = 0;
			KSPLAY_COUNTERS.composed_keys = 0;
			result = measure_ltr_scan(api, size);

			json_t* point = json_object();
			add_common_keys(point, "ltr_scan", size,
					FLAGS.measured_apis[i], result);
			if (FLAGS.measured_apis[i] == &dict_ksplay) {
				json_object_set_new(point, "ksplay_steps",
						json_integer(KSPLAY_COUNTERS.ksplay_steps));
				json_object_set_new(point, "ksplay_composed_keys",
						json_integer(KSPLAY_COUNTERS.composed_keys));
			}
			json_array_append_new(json_results, point);
			measurement_results_release(result.results);
		}

		for (int i = 0; FLAGS.measured_apis[i]; ++i) {
			// TODO: Only measure if there are enough words
			// in the file.
			result = measure_word_frequency(FLAGS.measured_apis[i],
					size);

			json_t* point = json_object();
			add_common_keys(point, "word_frequency", size,
					FLAGS.measured_apis[i], result);
			json_array_append_new(json_results, point);
			measurement_results_release(result.results);
		}

		log_verbose(1, "flushing results...");
		CHECK(!json_dump_file(json_results,
					"experiments/performance/output/results.json",
					JSON_INDENT(2)), "cannot dump results");
		log_verbose(1, "done");
	}


	deinit_word_frequency();
	json_decref(json_results);
	return 0;
}
