#ifndef HTABLE_HASH_H
#define HTABLE_HASH_H

#include <stdint.h>

#include "rand/rand.h"

// Some kind of hacky hash function called by the original htable.
// TODO: Get rid of this thing.
uint64_t fnv_hash(uint64_t key, uint64_t hash_max);

// Simple tabulation hashing, per-byte.
typedef struct {
	// Per-byte table of hashes.
	uint64_t table[sizeof(uint64_t)][256];
	uint64_t hash_max;
} sth;

void sth_init(sth* this, uint64_t hash_max, rand_generator* rand);
uint64_t sth_hash(const sth* this, uint64_t key);

#endif
