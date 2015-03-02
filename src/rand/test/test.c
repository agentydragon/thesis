#include "rand/test/test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rand/rand.h"
#include "log/log.h"

static void distribution_within(int hits, int buckets,
		int bucket_min, int bucket_max) {
	rand_generator generator = { .state = 0 };
	int* histogram = malloc(sizeof(int) * buckets);
	memset(histogram, 0, sizeof(int) * buckets);

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
	distribution_within(10000, 10, 900, 1100);
	distribution_within(10000000, 200, 49000, 51000);
}
