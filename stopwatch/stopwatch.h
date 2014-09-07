#ifndef STOPWATCH_H_INCLUDED
#define STOPWATCH_H_INCLUDED

#include <stdint.h>
#include <time.h>

typedef struct {
	struct timespec started_at;
} stopwatch;

stopwatch stopwatch_start();
uint64_t stopwatch_read_nsec(stopwatch);
uint64_t stopwatch_read_usec(stopwatch);
uint64_t stopwatch_read_msec(stopwatch);
void stopwatch_read_sec_msec(stopwatch, uint64_t* sec, uint64_t* msec);

#endif
