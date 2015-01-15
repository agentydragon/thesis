#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#include "../../veb_layout/veb_layout.h"
#include "../../measurement/measurement.h"
#include "../../stopwatch/stopwatch.h"

#include "../../log/log.h"
#include "../../rand/rand.h"

struct metrics {
	uint64_t cache_misses;
	uint64_t cache_references;
	uint64_t time_nsec;
};

struct metrics measure_for(uint64_t height, uint64_t N) {
	uint64_t tree_size = 1 << (height - 1);
	uint64_t* indices = calloc(N, sizeof(uint64_t));
	for (uint64_t i = 0; i < N; i++) {
		indices[i] = rand() % tree_size;
	}

	struct measurement measurement = measurement_begin();
	stopwatch watch = stopwatch_start();

	for (uint64_t i = 0; i < N; i++) {
		veb_pointer left, right;
		veb_get_children(indices[i], height, &left, &right);
	}

	struct measurement_results results = measurement_end(measurement);
	struct metrics tr = {
		.cache_misses = results.cache_misses,
		.cache_references = results.cache_references,
		.time_nsec = stopwatch_read_ns(watch)
	};
	return tr;
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;
	FILE* output = fopen("experiments/veb_performance/results.csv", "w");

	uint64_t N = 1000000;

	for (uint64_t height = 1; height < 50; height++) {
		struct metrics results = measure_for(height, N);

		fprintf(output,
				"%" PRIu64 "\t%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
				"\n",
				height, N,
				results.cache_misses, results.cache_references, results.time_nsec);
		fflush(output);
	}

	fclose(output);
	return 0;
}
