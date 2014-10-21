#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../../stopwatch/stopwatch.h"
#include "../../rand/rand.h"

/*
static void set_up(volatile uint8_t* memory, uint64_t size) {
	rand_generator generator;
	rand_seed_with_time(&generator);
	for (uint64_t i = 0; i < size; i++) {
		memory[i] = rand_next(&generator, 256);
	}
}
*/

struct result {
	uint64_t sequential_ns;
	uint64_t random_ns;

	uint64_t sequential_penalty_ns;
	uint64_t random_penalty_ns;
};

struct result measure_ns(uint64_t size) {
	uint64_t REPS = (512ULL * 1024ULL * 1024ULL) / size;
	if (REPS > 64ULL * 1024ULL) REPS = 64ULL * 1024ULL;
	volatile uint8_t* memory = malloc(size);

	struct result result;


	{
		stopwatch s = stopwatch_start();
		for (uint64_t i = 0; i < REPS; i++) {
			for (uint64_t j = 0; j < size; j++) {
				// Do nothing, but -O0.
			}
		}
		result.sequential_penalty_ns = stopwatch_read_ns(s) / REPS;
	}

	{
		stopwatch s = stopwatch_start();
		for (uint64_t i = 0; i < REPS; i++) {
			for (uint64_t j = 0; j < size; j++) {
				memory[j] = j;
			}
		}
		result.sequential_ns = stopwatch_read_ns(s) / REPS;
	}

	uint64_t offset = rand();
	while (offset % 2 == 0) offset = rand();
	// NOTE: offset is now coprime to any powers of 2.

	{
		stopwatch s = stopwatch_start();

		for (uint64_t i = 0; i < REPS; i++) {
			for (uint64_t j = 0, x = 0; j < size; j++) {
				x = (x + offset) % size;
			}
		}
		result.random_penalty_ns = stopwatch_read_ns(s) / REPS;
	}

	{
		stopwatch s = stopwatch_start();

		for (uint64_t i = 0; i < REPS; i++) {
			for (uint64_t j = 0, x = 0; j < size; j++) {
				memory[x] = j;
				x = (x + offset) % size;
			}
		}
		result.random_ns = stopwatch_read_ns(s) / REPS;
	}

	free((void*) memory);

	return result;
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	const uint64_t sizes[] = {
		                    1024ULL,
		             2ULL * 1024ULL,
		             4ULL * 1024ULL,
		             8ULL * 1024ULL,
		            16ULL * 1024ULL,
		            32ULL * 1024ULL,
		            64ULL * 1024ULL,
		           128ULL * 1024ULL,
		           256ULL * 1024ULL,
		           512ULL * 1024ULL,
		          1024ULL * 1024ULL,
		   2ULL * 1024ULL * 1024ULL,
		   4ULL * 1024ULL * 1024ULL,
		   8ULL * 1024ULL * 1024ULL,
		  16ULL * 1024ULL * 1024ULL,
		  32ULL * 1024ULL * 1024ULL,
		  64ULL * 1024ULL * 1024ULL,
		//1024ULL * 1024ULL * 1024ULL,
		0
	};

	printf("all results in nanoseconds\n");
	for (const uint64_t* size = sizes; *size; size++) {
		struct result result = measure_ns(*size);
		printf("size %" PRIu64 ": %3.3fx slower;"
				"\tsequential %" PRIu64 " (%" PRIu64 " - %" PRIu64 ")"
				" random %" PRIu64 " (%" PRIu64 " - %" PRIu64 ")\n",
				*size,

				((double) result.random_ns - result.random_penalty_ns) /
				((double) result.sequential_ns - result.sequential_penalty_ns),

				result.sequential_ns - result.sequential_penalty_ns,
				result.sequential_ns, result.sequential_penalty_ns,

				result.random_ns - result.random_penalty_ns,
				result.random_ns, result.random_penalty_ns);
	}

	return 0;
}
