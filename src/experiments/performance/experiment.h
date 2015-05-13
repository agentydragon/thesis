#ifndef EXPERIMENTS_PERFORMANCE_EXPERIMENT_H
#define EXPERIMENTS_PERFORMANCE_EXPERIMENT_H

#include <stdint.h>

#include "dict/dict.h"
#include "measurement/measurement.h"

struct metrics {
	measurement_results* results;
	uint64_t time_nsec;
};

uint64_t make_key(uint64_t i);
uint64_t make_value(uint64_t i);

dict* seed(const dict_api* api, uint64_t size);
dict* seed_bulk(uint64_t size);

void check_contains(dict* dict, uint64_t key, uint64_t value);
void check_not_contains(dict* dict, uint64_t key);

#endif
