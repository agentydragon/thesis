#include "hash.h"
#include <assert.h>

uint64_t hash_of(htable* this, uint64_t x) {
	assert(this->blocks_size > 0);

	if (this->hash_fn_override) {
		return this->hash_fn_override(this->hash_fn_override_opaque, x);
	} else {
		// FNV
		uint64_t result = 0xcbf29ce484222325;
		for (int i = 0; i < 8; i++) {
			result *= 0x100000001b3;
			result ^= x & 0xFF;
			x >>= 8;
		}

		return result % this->blocks_size;
	}
}

/*
static uint64_t hash_fn(uint64_t x, uint64_t max) {
	// TODO: very slow
	return ((x * PRIME_X) ^ (x * PRIME_Y)) % max;
}

static uint64_t hash_of(htable* this, uint64_t x) {
	uint64_t result = hash_fn(x, this->table_size);
	// log_info("hash(%" PRIu64 ") where size=%ld = %" PRIu64, x, this->table_size, result);
	return result;
}
*/
