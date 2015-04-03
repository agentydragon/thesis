#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "cobt/cobt.h"
#include "dict/cobt.h"
#include "dict/ksplay.h"
#include "experiments/performance/flags.h"
#include "experiments/performance/ltr_scan.h"
#include "experiments/performance/playback.h"
#include "experiments/performance/serial.h"
#include "experiments/performance/word_frequency.h"
#include "experiments/performance/working_set.h"
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
			result = measure_serial(FLAGS.measured_apis[i],
					SERIAL_BOTH, size);

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
			if (!FLAGS.measured_apis[i]->next) {
				// No implementation of ordering.
				// TODO: Make the test more idiomatic.
				continue;
			}
			KSPLAY_COUNTERS.ksplay_steps = 0;
			KSPLAY_COUNTERS.composed_keys = 0;
			result = measure_ltr_scan(FLAGS.measured_apis[i], size);

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

	// TODO: unhardcode path, maybe even move to a separate executable?
	recording* r = load_recording("/mnt/data/prvak/hash-ops.0x7f566e3cb0e0");
	for (int i = 0; FLAGS.measured_apis[i]; ++i) {
		log_info("running recording on %s", FLAGS.measured_apis[i]->name);
		struct metrics result;
		result = measure_recording(FLAGS.measured_apis[i], r);

		json_t* point = json_object();
		// TODO: Size shouldn't be a made up integer.
		// TODO: Experiments should be distinguished.
		add_common_keys(point, "recording", 42,
				FLAGS.measured_apis[i], result);
		json_array_append_new(json_results, point);
		measurement_results_release(result.results);
	}
	destroy_recording(r);

	log_verbose(1, "flushing results...");
	CHECK(!json_dump_file(json_results,
				"experiments/performance/output/results.json",
				JSON_INDENT(2)), "cannot dump results");
	log_verbose(1, "done");

	deinit_word_frequency();
	json_decref(json_results);
	return 0;
}
