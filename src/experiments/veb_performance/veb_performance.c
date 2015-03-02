#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "log/log.h"
#include "measurement/measurement.h"
#include "rand/rand.h"
#include "stopwatch/stopwatch.h"
#include "veb_layout/veb_layout.h"

struct metrics {
	uint64_t cache_misses;
	uint64_t cache_references;
	uint64_t time_nsec;
};

struct metrics measure_random_for(uint64_t height, uint64_t N) {
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

struct metrics measure_drilldown_for(uint64_t height, uint64_t N) {
	bool* random_decisions = calloc(N * height, sizeof(bool));
	for (uint64_t i = 0; i < N * height; i++) {
		random_decisions[i] = rand() % 2;
	}

	struct measurement measurement = measurement_begin();
	stopwatch watch = stopwatch_start();

	for (uint64_t i = 0; i < N; i++) {
		uint64_t node = 0;
		for (uint64_t j = 1; j < height; j++) {
			veb_pointer left, right;
			veb_get_children(node, height, &left, &right);

			assert(left.present && right.present);
			if (random_decisions[i * height + j]) {
				node = left.node;
			} else {
				node = right.node;
			}
		}
	}

	struct measurement_results results = measurement_end(measurement);
	struct metrics tr = {
		.cache_misses = results.cache_misses,
		.cache_references = results.cache_references,
		.time_nsec = stopwatch_read_ns(watch)
	};
	return tr;
}

struct metrics measure_hyperdrilldown_for(uint64_t height, uint64_t N) {
	bool* random_decisions = calloc(N * height, sizeof(bool));
	for (uint64_t i = 0; i < N * height; i++) {
		random_decisions[i] = rand() % 2;
	}

	struct level_data ld[60];
	veb_prepare(height, ld);

	struct measurement measurement = measurement_begin();
	stopwatch watch = stopwatch_start();

	for (uint64_t i = 0; i < N; i++) {
		uint64_t node = 0;
		struct drilldown_track track;
		drilldown_begin(&track);
		for (uint64_t j = 1; j < height; j++) {
			if (random_decisions[i * height + j]) {
				drilldown_go_left(ld, &track);
				node = track.pos[track.depth];
			} else {
				drilldown_go_right(ld, &track);
				node = track.pos[track.depth];
			}
		}
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

	log_info("measuring accesses to random nodes (i.e. leaf-biased)");
	FILE* output = fopen("experiments/veb_performance/results-random.csv", "w");

	const uint64_t N = 500000;

	for (uint64_t height = 1; height < 50; height++) {
		struct metrics results = measure_random_for(height, N);

		fprintf(output,
				"%" PRIu64 "\t%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
				"\n",
				height, N,
				results.cache_misses, results.cache_references, results.time_nsec);
		fflush(output);
	}

	fclose(output);

	log_info("measuring drilldowns");
	output = fopen("experiments/veb_performance/results-drilldown.csv", "w");

	for (uint64_t height = 1; height < 50; height++) {
		struct metrics results = measure_drilldown_for(height, N);

		fprintf(output,
				"%" PRIu64 "\t%" PRIu64 "\t"
				"%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t"
				"\n",
				height, N,
				results.cache_misses, results.cache_references, results.time_nsec);
		fflush(output);
	}

	fclose(output);

	log_info("measuring hyperdrilldowns");
	output = fopen("experiments/veb_performance/results-hyperdrilldown.csv", "w");

	for (uint64_t height = 1; height < 50; height++) {
		struct metrics results = measure_hyperdrilldown_for(height, N);

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
