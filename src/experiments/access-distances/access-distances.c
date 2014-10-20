#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../../stopwatch/stopwatch.h"
#include "../../rand/rand.h"

static void clear(uint8_t* memory, const uint64_t size) {
	rand_generator generator;
	rand_seed_with_time(&generator);
	for (uint64_t i = 0; i < size; i++) {
		memory[i] = rand_next(&generator, 256);
	}
}

struct result {
	double interval_scan_ratio;
	double interval_bounds_ratio;
};

static struct result try_with_span(uint8_t* memory, const uint64_t size, const uint64_t span) {
	const uint64_t N = 1000000;
	uint64_t time1, time2, time3;
	uint64_t t1 = 0, t2 = 0, t3 = 0;
	struct result result;
	rand_generator generator;
	{
		// clear(memory, size);
		{
			generator.state = 0;
			stopwatch s = stopwatch_start();

			for (uint64_t i = 0; i < N; i++) {
				uint64_t offset = rand_next(&generator, size - span);
				t1 += memory[offset];
				// next_random(&scratch, span + 1);
			}
			time1 = stopwatch_read_ns(s);
		}

		// clear(memory, size);
		{
			generator.state = 0;
			stopwatch s = stopwatch_start();

			for (uint64_t i = 0; i < N; i++) {
				uint64_t offset = rand_next(&generator, size - span);

				for (uint64_t j = 0; j < span; j++) {
					t2 += memory[offset + j];
				}
				// t2 += memory[offset + next_random(&scratch, span + 1)];
			}

			time2 = stopwatch_read_ns(s);
		}

		// clear(memory, size);
		{
			generator.state = 0;
			stopwatch s = stopwatch_start();

			for (uint64_t i = 0; i < N; i++) {
				uint64_t offset = rand_next(&generator, size - span);
				t3 += memory[offset] * memory[offset + span];
			}

			time3 = stopwatch_read_ns(s);
		}
	}

	result.interval_scan_ratio = (double) time2 / (double) time1;
	result.interval_bounds_ratio = (double) time3 / (double) time1;

	printf("span %10ld: scan %2.2lf, bounds %2.2f (t1:%d,t2:%d,t3:%d)\n",
			span,
			result.interval_scan_ratio,
			result.interval_bounds_ratio,
			t1 % 10, t2 % 10, t3 % 10); // do not optimize away

	return result;
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	// const uint64_t size = 1024 * 1024 * 1024;
	// const uint64_t size = 16 * 1024 * 1024;
	// const uint64_t size = 1024 * 1024;
	const uint64_t size = 1024;
	uint8_t* memory = malloc(size);

	clear(memory, size);

	const int freq = 5;

	FILE* csv = fopen("findings.tsv", "w");
	const uint64_t max_span = 1024;
	for (uint64_t span = 1; span < max_span;) {
		struct result result = try_with_span(memory, size, span);

		fprintf(csv, "%ld\t%8.2f\t%8.2f\n", span,
				result.interval_scan_ratio,
				result.interval_bounds_ratio);
		fflush(csv);

		if (span < (1 << freq)) span += (1L << 0L);
		else span += span >> freq;

//		span += 1;
	}
	fclose(csv);
}
