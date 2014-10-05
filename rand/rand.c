#ifndef RAND_C
#define RAND_C

#include "rand.h"

#include <assert.h>
#include <time.h>

void rand_seed_with_time(rand_generator* generator) {
	struct timespec now;
	assert(clock_gettime(CLOCK_REALTIME, &now) == 0);
	generator->state = now.tv_sec * 1000000000LL + now.tv_nsec;
}

uint64_t rand_next(rand_generator* generator, uint64_t max) {
	uint64_t state = generator->state;
	state ^= state >> 12ULL;
	state ^= state << 25ULL;
	state ^= state >> 27ULL;
	generator->state = state + 1;

	return (state * 2685821657736338717ULL) % max;
}

#endif
