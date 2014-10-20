#include "test.h"
#include "../observation.h"
#include "../../hash_array/hash_array.h"
#include "../../log/log.h"

#include <stdlib.h>
#include <assert.h>

static void run_events(observation* observation) {
	hash* example;
	// TODO: use an actual test mock that doesn't do anything?
	if (hash_init(&example, &hash_array, NULL))
		log_fatal("cannot init test example hash");

	hash* tapped_hash;
	observation_tap(observation, example, &tapped_hash);

	assert(!hash_find(tapped_hash, 1, NULL, NULL));
	assert(!hash_find(tapped_hash, 2, NULL, NULL));
	assert(!hash_insert(tapped_hash, 1, 100));
	assert(!hash_insert(tapped_hash, 2, 250));
	assert(!hash_find(tapped_hash, 2, NULL, NULL));
	assert(!hash_delete(tapped_hash, 2));
	assert(!hash_insert(tapped_hash, 2, 200));
	assert(!hash_find(tapped_hash, 2, NULL, NULL));
	assert(!hash_insert(tapped_hash, 3, 300));
	assert(!hash_delete(tapped_hash, 3));

	hash_destroy(&tapped_hash);
	hash_destroy(&example);
}

static void replay_events(observation* observation) {
	hash* replay_on;
	// TODO: use message expectations instead?
	if (hash_init(&replay_on, &hash_array, NULL))
		log_fatal("cannot init test replay hash");

	observation_replay(observation, replay_on);

	uint64_t value;
	bool found;
	assert(!hash_find(replay_on, 1, &value, &found));
	assert(found && value == 100);
	assert(!hash_find(replay_on, 2, &value, &found));
	assert(found && value == 200);
	assert(!hash_find(replay_on, 3, &value, &found));
	assert(!found);

	hash_destroy(&replay_on);
}

static void it_delegates() {
	observation* observation;
	if (observation_init(&observation))
		log_fatal("cannot init observation");

	run_events(observation);
	replay_events(observation);

	observation_destroy(&observation);
}

void test_observation() {
	it_delegates();
}
