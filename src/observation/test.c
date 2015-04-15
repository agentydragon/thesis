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

	dict_find(tapped_dict, 1, NULL, NULL);
	dict_find(tapped_dict, 2, NULL, NULL);
	ASSERT(!dict_insert(tapped_dict, 1, 100));
	ASSERT(!dict_insert(tapped_dict, 2, 250));
	dict_find(tapped_dict, 2, NULL, NULL);
	ASSERT(!dict_delete(tapped_dict, 2));
	ASSERT(!dict_insert(tapped_dict, 2, 200));
	dict_find(tapped_dict, 2, NULL, NULL);
	ASSERT(!dict_insert(tapped_dict, 3, 300));
	ASSERT(!dict_delete(tapped_dict, 3));

	dict_destroy(&tapped_dict);
	dict_destroy(&example);
}

static void replay_events(observation* observation) {
	dict* replay_on;
	// TODO: use message expectations instead?
	CHECK(dict_init(&replay_on, &dict_array, NULL),
			"cannot init test replay dict");

	observation_replay(observation, replay_on);

	uint64_t value;
	bool found;
	dict_find(replay_on, 1, &value, &found);
	ASSERT(found && value == 100);
	dict_find(replay_on, 2, &value, &found);
	ASSERT(found && value == 200);
	dict_find(replay_on, 3, &value, &found);
	ASSERT(!found);

	dict_destroy(&replay_on);
}

static void it_delegates(void) {
	observation* observation;
	CHECK(observation_init(&observation), "cannot init observation");

	run_events(observation);
	replay_events(observation);

	observation_destroy(&observation);
}

void test_observation(void) {
	it_delegates();
}
