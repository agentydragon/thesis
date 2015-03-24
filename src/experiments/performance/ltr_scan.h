#ifndef EXPERIMENTS_PERFORMANCE_LTR_SCAN_H
#define EXPERIMENTS_PERFORMANCE_LTR_SCAN_H

#include "experiments/performance/experiment.h"
#include "dict/dict.h"

struct metrics measure_ltr_scan(const dict_api* api, uint64_t size);

#endif
