#include "experiments/performance/experiment.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include "dict/array.h"
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
	CHECK(dict_insert(dict, key, value), "cannot insert");
}

void check_contains(dict* dict, uint64_t key, uint64_t value) {
	uint64_t found_value;
	ASSERT(dict_find(dict, key, &found_value) && found_value == value);
}

void check_not_contains(dict* dict, uint64_t key) {
	ASSERT(!dict_find(dict, key, NULL));
}

dict* seed(const dict_api* api, uint64_t size) {
	dict* dict;
	dict_init(&dict, api);
	for (uint64_t i = 0; i < size; i++) {
		insert(dict, make_key(i), make_value(i));
	}
	return dict;
}

struct pair { uint64_t key, value; };

static int cmp_pair(const void* _x, const void* _y) {
	struct pair const *x = _x, *y = _y;
	if (x->key < y->key) {
		return -1;
	} else if (x->key == y->key) {
		return 0;
	} else {
		return 1;
	}
}

// Like plain 'seed', but runs faster (since it doesn't insert at random).
dict* seed_bulk(const dict_api* api, uint64_t size) {
	dict* dict;
	dict_init(&dict, api);

	if (api == &dict_array) {
		// Arrays are fast when we insert in sorted order.
		struct pair *pairs;
		pairs = calloc(size, sizeof(struct pair));
		ASSERT(pairs);
		for (uint64_t i = 0; i < size; i++) {
			pairs[i].key = make_key(i);
			pairs[i].value = make_value(i);
		}
		qsort(pairs, size, sizeof(struct pair), cmp_pair);
		for (uint64_t i = 0; i < size; i++) {
			insert(dict, pairs[i].key, pairs[i].value);
		}
		free(pairs);
	} else {
		// No change in other structures.
		for (uint64_t i = 0; i < size; i++) {
			insert(dict, make_key(i), make_value(i));
		}
	}
	return dict;
}
