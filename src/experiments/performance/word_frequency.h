#ifndef EXPERIMENTS_PERFORMANCE_WORD_FREQUENCY_H
#define EXPERIMENTS_PERFORMANCE_WORD_FREQUENCY_H

#include "experiments/performance/experiment.h"
#include "dict/dict.h"

struct metrics measure_word_frequency(const dict_api* api, uint64_t size);

#endif
