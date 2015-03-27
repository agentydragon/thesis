#include "measurement/stopwatch.h"

#include "log/log.h"

stopwatch stopwatch_start() {
	stopwatch this;
	ASSERT(clock_gettime(CLOCK_REALTIME, &this.started_at) == 0);
	return this;
}

static uint64_t timespec_diff_ns(struct timespec t1, struct timespec t2) {
	return
		(t1.tv_sec * 1000000000LL + t1.tv_nsec) -
		(t2.tv_sec * 1000000000LL + t2.tv_nsec);
}

uint64_t stopwatch_read_ns(stopwatch this) {
	struct timespec now;
	ASSERT(clock_gettime(CLOCK_REALTIME, &now) == 0);
	return timespec_diff_ns(now, this.started_at);
}

uint64_t stopwatch_read_us(stopwatch this) {
	return stopwatch_read_ns(this) / 1000;
}

uint64_t stopwatch_read_ms(stopwatch this) {
	return stopwatch_read_us(this) / 1000;
}

void stopwatch_read_s_ms(stopwatch this, uint64_t* s, uint64_t* ms) {
	uint64_t ns = stopwatch_read_ns(this);
	*ms = (ns / 1000000) % 1000;
	*s = (ns / 1000000) / 1000;
}
