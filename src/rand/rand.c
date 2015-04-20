#ifndef RAND_C
#define RAND_C

#include "rand/rand.h"

#include "log/log.h"

#include <assert.h>
#include <time.h>

void rand_seed_with_time(rand_generator* generator) {
	struct timespec now;
	ASSERT(clock_gettime(CLOCK_REALTIME, &now) == 0);
	generator->state = now.tv_sec * 1000000000LL + now.tv_nsec;
}

// TODO: Cite the generator over here.
// TODO: More internal state may be useful...

static uint64_t advance(rand_generator* generator) {
	uint64_t state = generator->state;
	state ^= state >> 12ULL;
	state ^= state << 25ULL;
	state ^= state >> 27ULL;
	generator->state = state + 1;
	return state;
}

uint64_t rand_next64(rand_generator* generator) {
	// TODO: This may be too predicable...
	return advance(generator) * 2685821657736338717ULL;
}

uint64_t rand_next(rand_generator* generator, uint64_t max) {
	return rand_next64(generator) % max;
}

#endif
