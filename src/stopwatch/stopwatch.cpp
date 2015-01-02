#include "stopwatch.h"

#include <assert.h>

stopwatch stopwatch_start() {
	stopwatch _this;
	assert(clock_gettime(CLOCK_REALTIME, &_this.started_at) == 0);
	return _this;
}

static uint64_t timespec_diff_ns(struct timespec t1, struct timespec t2) {
	return
		(t1.tv_sec * 1000000000LL + t1.tv_nsec) -
		(t2.tv_sec * 1000000000LL + t2.tv_nsec);
}

uint64_t stopwatch_read_ns(stopwatch _this) {
	struct timespec now;
	assert(clock_gettime(CLOCK_REALTIME, &now) == 0);
	return timespec_diff_ns(now, _this.started_at);
}

uint64_t stopwatch_read_us(stopwatch _this) {
	return stopwatch_read_ns(_this) / 1000;
}

uint64_t stopwatch_read_ms(stopwatch _this) {
	return stopwatch_read_us(_this) / 1000;
}

void stopwatch_read_s_ms(stopwatch _this, uint64_t* s, uint64_t* ms) {
	uint64_t ns = stopwatch_read_ns(_this);
	*ms = (ns / 1000000) % 1000;
	*s = (ns / 1000000) / 1000;
}
