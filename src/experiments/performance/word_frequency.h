#ifndef EXPERIMENTS_PERFORMANCE_WORD_FREQUENCY_H
#define EXPERIMENTS_PERFORMANCE_WORD_FREQUENCY_H

#include "experiments/performance/experiment.h"
#include "dict/dict.h"

void init_word_frequency(void);
void deinit_word_frequency(void);
struct metrics measure_word_frequency(const dict_api* api, uint64_t size);

#endif
