#ifndef HTABLE_HASH_H
#define HTABLE_HASH_H

#include <stdint.h>

#include "rand/rand.h"

// Some kind of hacky hash function called by the original htable.
// TODO: Get rid of this thing.
uint64_t fnv_hash(uint64_t key, uint64_t hash_max);

#endif
