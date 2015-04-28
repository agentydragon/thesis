#ifndef UTIL_FNV_H
#define UTIL_FNV_H

#include <stdint.h>

typedef struct {
	uint64_t hash;
} fnv1_state;

// 64-bit FNV-1
fnv1_state fnv1_begin();
void fnv1_add(fnv1_state* state, uint8_t octet);
uint64_t fnv1_finish(fnv1_state state);

uint64_t fnv1_hash_str(const char* word);

#endif
