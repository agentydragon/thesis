#ifndef EXPERIMENTS_PERFORMANCE_SERIAL_H
#define EXPERIMENTS_PERFORMANCE_SERIAL_H

#include "experiments/performance/experiment.h"

// TODO: kill SERIAL_BOTH
typedef enum { SERIAL_BOTH, SERIAL_JUST_FIND, SERIAL_JUST_INSERT } serial_mode;
struct metrics measure_serial(const dict_api* api, serial_mode mode,
		uint64_t size, uint64_t success_percentage);

#endif
