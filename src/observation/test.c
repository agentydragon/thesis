#include "observation/test.h"

#include <assert.h>
#include <stdlib.h>

#include "dict/array.h"
#include "log/log.h"
#include "observation/observation.h"

static void run_events(observation* observation) {
	dict* example;
	// TODO: use an actual test mock that doesn't do anything?
	if (dict_init(&example, &dict_array, NULL))
		log_fatal("cannot init test example dict");

	dict* tapped_dict;
	observation_tap(observation, example, &tapped_dict);

	assert(!dict_find(tapped_dict, 1, NULL, NULL));
	assert(!dict_find(tapped_dict, 2, NULL, NULL));
	assert(!dict_insert(tapped_dict, 1, 100));
	assert(!dict_insert(tapped_dict, 2, 250));
	assert(!dict_find(tapped_dict, 2, NULL, NULL));
	assert(!dict_delete(tapped_dict, 2));
	assert(!dict_insert(tapped_dict, 2, 200));
	assert(!dict_find(tapped_dict, 2, NULL, NULL));
	assert(!dict_insert(tapped_dict, 3, 300));
	assert(!dict_delete(tapped_dict, 3));

	dict_destroy(&tapped_dict);
	dict_destroy(&example);
}

static void replay_events(observation* observation) {
	dict* replay_on;
	// TODO: use message expectations instead?
	if (dict_init(&replay_on, &dict_array, NULL))
		log_fatal("cannot init test replay dict");

	observation_replay(observation, replay_on);

	uint64_t value;
	bool found;
	assert(!dict_find(replay_on, 1, &value, &found));
	assert(found && value == 100);
	assert(!dict_find(replay_on, 2, &value, &found));
	assert(found && value == 200);
	assert(!dict_find(replay_on, 3, &value, &found));
	assert(!found);

	dict_destroy(&replay_on);
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
