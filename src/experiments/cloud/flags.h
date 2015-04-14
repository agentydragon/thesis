#ifndef EXPERIMENTS_CLOUD_FLAGS_H
#define EXPERIMENTS_CLOUD_FLAGS_H

#include "dict/dict.h"

struct {
	const dict_api* measured_apis[20];
	bool dump_averages;

	enum { SCATTER_GRID, SCATTER_RANDOM } scatter;
	uint64_t lat_step;
	uint64_t lon_step;
	uint64_t sample_count;

	int min_year;  // inclusive
	int max_year;  // inclusive

	// How many next/prev queries should we perform per sample.
	uint64_t close_point_count;
} FLAGS;

void parse_flags(int argc, char** argv);

#endif
