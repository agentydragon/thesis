#include "hash.h"
#include <assert.h>

uint64_t fnv_hash(uint64_t key, uint64_t hash_max) {
	uint64_t result = 0xcbf29ce484222325;
	for (int i = 0; i < 8; i++) {
		result *= 0x100000001b3;
		result ^= key & 0xFF;
		key >>= 8;
	}
	return result % hash_max;
}
