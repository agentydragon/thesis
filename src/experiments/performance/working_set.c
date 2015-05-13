#include "experiments/performance/working_set.h"

#include "measurement/measurement.h"
#include "measurement/stopwatch.h"
#include "rand/rand.h"

struct metrics measure_working_set(const dict_api* api, uint64_t size,
		uint64_t working_set_size) {
	const uint64_t used_ws_size =
			working_set_size < size ? working_set_size : size;

	dict* table = seed_bulk(api, size);

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
