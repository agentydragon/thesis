#ifndef HASH_HASHTABLE_PRIVATE_HASH_H
#define HASH_HASHTABLE_PRIVATE_HASH_H

#include "data.h"

uint64_t hashtable_hash_of(struct hashtable_data* this, uint64_t key);
// uint64_t hash_of(struct hashtable_data* this, uint64_t x);

#endif
