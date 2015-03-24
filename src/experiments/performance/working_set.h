#ifndef EXPERIMENTS_PERFORMANCE_WORKING_SET_H
#define EXPERIMENTS_PERFORMANCE_WORKING_SET_H

#include "experiments/performance/experiment.h"

struct metrics measure_working_set(const dict_api* api, uint64_t size,
		uint64_t working_set_size);

#endif
