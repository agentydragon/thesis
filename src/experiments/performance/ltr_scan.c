#include "experiments/performance/ltr_scan.h"

#include <assert.h>

#include "log/log.h"
#include "measurement/stopwatch.h"
#include "util/consume.h"

static void iterate_ltr(dict* dict) {
	uint64_t min = 0;
	bool found = dict_find(dict, min, NULL);
	if (!found) {
		found = dict_next(dict, min, &min);
	}

	uint64_t current_key = min;
	while (found) {
		uint64_t value;
		ASSERT(dict_find(dict, current_key, &value));
		consume64(value);

		uint64_t next_key;
		found = dict_next(dict, current_key, &next_key);
		if (found) {
			ASSERT(next_key > current_key);
		}
		current_key = next_key;
	}
}

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
