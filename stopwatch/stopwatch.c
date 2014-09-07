#include "stopwatch.h"

#include <assert.h>

stopwatch stopwatch_start() {
	stopwatch this;
	assert(clock_gettime(CLOCK_REALTIME, &this.started_at) == 0);
	return this;
}

static uint64_t timespec_diff_nsec(struct timespec t1, struct timespec t2) {
	return
		(t1.tv_sec * 1000000000LL + t1.tv_nsec) -
		(t2.tv_sec * 1000000000LL + t2.tv_nsec);
}

uint64_t stopwatch_read_nsec(stopwatch this) {
	struct timespec now;
	assert(clock_gettime(CLOCK_REALTIME, &now) == 0);
	return timespec_diff_nsec(now, this.started_at);
}

uint64_t stopwatch_read_usec(stopwatch this) {
	return stopwatch_read_nsec(this) / 1000;
}

uint64_t stopwatch_read_msec(stopwatch this) {
	return stopwatch_read_usec(this) / 1000;
}

void stopwatch_read_sec_msec(stopwatch this, uint64_t* sec, uint64_t* msec) {
	uint64_t nsec = stopwatch_read_nsec(this);
	*msec = (nsec / 1000000) % 1000;
	*sec = (nsec / 1000000) / 1000;
}
