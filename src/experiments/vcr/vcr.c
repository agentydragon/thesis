#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "experiments/vcr/flags.h"
#include "experiments/vcr/playback.h"
#include "log/log.h"
#include "measurement/measurement.h"

int main(int argc, char** argv) {
	parse_flags(argc, argv);

	recording* record = load_recording(FLAGS.recording_path);

	json_t* json_results = json_array();
	uint64_t times[20];
	for (int i = 0; FLAGS.measured_apis[i]; ++i) {
		const dict_api* api = FLAGS.measured_apis[i];
		log_info("running recording on %s", api->name);
		struct metrics result;
		result = measure_recording(api, record);

		json_t* point = json_object();
		json_object_set_new(point, "implementation",
				json_string(api->name));
		json_object_set_new(point, "metrics",
				measurement_results_to_json(result.results));
		json_object_set_new(point, "time_ns",
				json_integer(result.time_nsec));
		json_array_append_new(json_results, point);
		measurement_results_release(result.results);

		times[i] = result.time_nsec;
	}
	destroy_recording(record);

	CHECK(!json_dump_file(json_results,
				"experiments/vcr/results.json",
				JSON_INDENT(2)), "cannot dump results");
	json_decref(json_results);

	for (int i = 0; FLAGS.measured_apis[i]; ++i) {
		log_info("%30s: %14" PRIu64 " ns", FLAGS.measured_apis[i]->name,
				times[i]);
	}

//	char buffer[1024];
//	strcpy(buffer, "\\texttt{}");
//	for (int i = 0; FLAGS.measured_apis[i]; ++i) {
//		sprintf(buffer + strlen(buffer), " & %" PRIu64, times[i] / (1000 * 1000));
//	}
//	sprintf(buffer + strlen(buffer), "\n");
//	log_info("\n%s\n", buffer);

	return 0;
}
