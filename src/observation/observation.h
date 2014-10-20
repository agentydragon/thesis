#ifndef OBSERVATION_H
#define OBSERVATION_H

#include "../hash/hash.h"

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

// TODO: maybe observation_tap_destructive(observation*, hash** tap_to_tapped)?
int8_t observation_tap(observation*, hash* to_tap, hash** tapped);
int8_t observation_replay(observation*, hash* replay_on);

/*
int8_t observation_load(observation*, const char* filename);
int8_t observation_save(observation*, const char* filename);
*/

#endif
