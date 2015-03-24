#ifndef EXPERIMENTS_PERFORMANCE_SERIAL_H
#define EXPERIMENTS_PERFORMANCE_SERIAL_H

#include "experiments/performance/experiment.h"

typedef enum { SERIAL_BOTH, SERIAL_JUST_FIND } serial_mode;
struct metrics measure_serial(const dict_api* api, serial_mode mode,
		uint64_t size);

#endif
