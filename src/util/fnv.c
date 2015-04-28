#include "util/fnv.h"

fnv1_state fnv1_begin() {
	return (fnv1_state) {
		.hash = 14695981039346656037ULL
	};
}

void fnv1_add(fnv1_state* state, uint8_t octet) {
	state->hash = (state->hash * 109951162811LL) ^ octet;
}

uint64_t fnv1_finish(fnv1_state state) {
	return state.hash;
}

uint64_t fnv1_hash_str(const char* word) {
	fnv1_state state = fnv1_begin();
	for (const char* p = word; *p; p++) {
		fnv1_add(&state, *p);
	}
	return fnv1_finish(state);
}
