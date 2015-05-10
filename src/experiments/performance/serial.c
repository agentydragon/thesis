#include "experiments/performance/serial.h"

#include "log/log.h"
#include "measurement/stopwatch.h"
#include "rand/rand.h"

struct metrics measure_serial(const dict_api* api, serial_mode mode,
		uint64_t size, uint64_t success_percentage) {
	measurement* measurement_both = measurement_begin();
	measurement* measurement_just_insert = measurement_begin();
	stopwatch watch = stopwatch_start();
	measurement_results *results_just_insert;
	uint64_t time_just_insert_ns;

	dict* table = seed(api, size);
	results_just_insert = measurement_end(measurement_just_insert);
	time_just_insert_ns = stopwatch_read_ns(watch);

	measurement* measurement_just_find = measurement_begin();
	stopwatch watch_just_find = stopwatch_start();

	rand_generator generator = { .state = 0 };
	// TODO: separate size/reads
	for (uint64_t i = 0; i < size; i++) {
		const int k = rand_next(&generator, size);
		// Let every read be a hit.
		if (rand_next(&generator, 100) < success_percentage) {
			check_contains(table, make_key(k), make_value(k));
		} else {
			check_not_contains(table, make_key(k + size));
		}
	}

	measurement_results *results_combined = measurement_end(measurement_both),
			*results_just_find = measurement_end(measurement_just_find);
	dict_destroy(&table);

	switch (mode) {
	case SERIAL_BOTH:
		measurement_results_release(results_just_find);
		measurement_results_release(results_just_insert);
		return (struct metrics) {
			.results = results_combined,
			.time_nsec = stopwatch_read_ns(watch)
		};
	case SERIAL_JUST_INSERT:
		measurement_results_release(results_combined);
		measurement_results_release(results_just_find);
		return (struct metrics) {
			.results = results_just_insert,
			.time_nsec = time_just_insert_ns
		};
	case SERIAL_JUST_FIND:
		measurement_results_release(results_combined);
		measurement_results_release(results_just_insert);
		return (struct metrics) {
			.results = results_just_find,
			.time_nsec = stopwatch_read_ns(watch_just_find)
		};
	default:
		log_fatal("internal error");
	}
}
