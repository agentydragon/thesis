#ifndef EXPERIMENTS_VCR_PLAYBACK_H
#define EXPERIMENTS_VCR_PLAYBACK_H

#include <stdint.h>

#include "dict/dict.h"
#include "measurement/measurement.h"

// TODO: Copied from experiments/performance
struct metrics {
	measurement_results* results;
	uint64_t time_nsec;
};

typedef enum { FIND, INSERT, DELETE } operation_type;

typedef struct {
	uint64_t key;
	operation_type operation;
} recorded_operation;

typedef struct {
	recorded_operation* operations;
	uint64_t capacity;
	uint64_t length;
} recording;

recording* load_recording(const char* path);
void destroy_recording(recording*);
struct metrics measure_recording(const dict_api* api, recording* record,
		int repetitions);


#endif
