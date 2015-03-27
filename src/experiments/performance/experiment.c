#include "experiments/performance/experiment.h"

#include <assert.h>
#include <stdlib.h>

#include "dict/test/toycrypt.h"
#include "log/log.h"
#include "rand/rand.h"

uint64_t make_key(uint64_t i) {
	return toycrypt(i, 0x0123456789ABCDEFLL);
}

uint64_t make_value(uint64_t i) {
	return toycrypt(i, 0xFEDCBA9876543210LL);
}

static void insert(dict* dict, uint64_t key, uint64_t value) {
	CHECK(key != DICT_RESERVED_KEY, "trying to insert reserved key");
	CHECK(!dict_insert(dict, key, value), "cannot insert");
}

void check_contains(dict* dict, uint64_t key, uint64_t value) {
	uint64_t found_value;
	bool found;
	ASSERT(!dict_find(dict, key, &found_value, &found));
	ASSERT(found && found_value == value);
}

dict* seed(const dict_api* api, uint64_t size) {
	dict* dict;
	CHECK(!dict_init(&dict, api, NULL), "cannot init dict");
	for (uint64_t i = 0; i < size; i++) {
		insert(dict, make_key(i), make_value(i));
	}
	return dict;
}
