#include "test_rand.h"
#include "../rand/rand.h"
#include "../log/log.h"
#include "../stopwatch/stopwatch.h"

#include <stdio.h>
#include <stdlib.h>

#define it(behavior) do { \
	printf("  %40s ", #behavior); \
	\
	stopwatch watch = stopwatch_start(); \
	behavior; \
	printf("OK %ld us\n", stopwatch_read_ns(watch)); \
} while (0)

static void distribution_within(int hits, int buckets,
		int bucket_min, int bucket_max) {
	rand_generator generator = { .state = 0 };
	int* histogram = malloc(sizeof(int) * buckets);

	for (int i = 0; i < hits; i++) {
		histogram[rand_next(&generator, buckets)]++;
	}

	for (int i = 0; i < buckets; i++) {
		if (histogram[i] < bucket_min || histogram[i] > bucket_max) {
			log_fatal("bucket %d of size %d out of range (%d..%d)",
					i, histogram[i],
					bucket_min, bucket_max);
		}
	}
	free(histogram);
}

void test_rand() {
	it(distribution_within(10000, 10, 900, 1100));
	it(distribution_within(10000000, 200, 49000, 51000));
}
