#ifndef STOPWATCH_H_INCLUDED
#define STOPWATCH_H_INCLUDED

#include <stdint.h>
#include <time.h>

typedef struct {
	struct timespec started_at;
} stopwatch;

stopwatch stopwatch_start();
uint64_t stopwatch_read_ns(stopwatch);
uint64_t stopwatch_read_us(stopwatch);
uint64_t stopwatch_read_ms(stopwatch);
void stopwatch_read_s_ms(stopwatch, uint64_t* sec, uint64_t* msec);

#endif
