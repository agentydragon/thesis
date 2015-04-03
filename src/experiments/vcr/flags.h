#ifndef EXPERIMENTS_VCR_FLAGS_H
#define EXPERIMENTS_VCR_FLAGS_H

#include <inttypes.h>
#include "dict/dict.h"

struct {
	const dict_api* measured_apis[20];
	char* recording_path;
} FLAGS;

void parse_flags(int argc, char** argv);

#endif
