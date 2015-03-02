#ifndef OBSERVATION_H
#define OBSERVATION_H

#include "dict/dict.h"

struct observed_operation {
	enum { OP_FIND, OP_INSERT, OP_DELETE } operation;

	union {
		struct {
			uint64_t key;
		} find;

		struct {
			uint64_t key;
			uint64_t value;
		} insert;

		struct {
			uint64_t key;
		} delete;
	};
};

typedef struct observation observation;

int8_t observation_init(observation**);
void observation_destroy(observation**);

// TODO: maybe observation_tap_destructive(observation*, dict** tap_to_tapped)?
int8_t observation_tap(observation*, dict* to_tap, dict** tapped);
int8_t observation_replay(observation*, dict* replay_on);

/*
int8_t observation_load(observation*, const char* filename);
int8_t observation_save(observation*, const char* filename);
*/

#endif
