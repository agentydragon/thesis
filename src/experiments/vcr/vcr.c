#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "experiments/vcr/flags.h"
#include "experiments/vcr/playback.h"
#include "log/log.h"
#include "measurement/measurement.h"

int main(int argc, char** argv) {
	parse_flags(argc, argv);

	json_t* json_results = json_array();

	// TODO: unhardcode path
	recording* r = load_recording(FLAGS.recording_path); // "/mnt/data/prvak/hash-ops.0x7f566e3cb0e0");
	for (int i = 0; FLAGS.measured_apis[i]; ++i) {
		const dict_api* api = FLAGS.measured_apis[i];
		log_info("running recording on %s", api->name);
		struct metrics result;
		result = measure_recording(api, r);

		json_t* point = json_object();
		// TODO: Size shouldn't be a made up integer.
		// TODO: Experiments should be distinguished.
		json_object_set_new(point, "implementation",
				json_string(api->name));
		json_object_set_new(point, "metrics",
				measurement_results_to_json(result.results));
		json_object_set_new(point, "time_ns",
				json_integer(result.time_nsec));
		json_array_append_new(json_results, point);
		measurement_results_release(result.results);
	}
	destroy_recording(r);

	log_verbose(1, "flushing results...");
	CHECK(!json_dump_file(json_results,
				"experiments/vcr/results.json",
				JSON_INDENT(2)), "cannot dump results");
	log_verbose(1, "done");

	json_decref(json_results);
	return 0;
}
