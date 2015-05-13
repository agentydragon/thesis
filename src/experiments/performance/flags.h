#ifndef EXPERIMENTS_PERFORMANCE_FLAGS_H
#define EXPERIMENTS_PERFORMANCE_FLAGS_H

#include <inttypes.h>
#include "dict/dict.h"

struct {
	uint64_t minimum;
	uint64_t maximum;
	double base;
	const dict_api* measured_apis[20];
} FLAGS;

void parse_flags(int argc, char** argv);

#endif
