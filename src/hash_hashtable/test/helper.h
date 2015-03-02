#ifndef HASH_HASHTABLE_HELPER_H
#define HASH_HASHTABLE_HELPER_H

#include "hash_hashtable/private/data.h"

#define hash_mock hash_hashtable_test_hash_mock
#define AMEN HASH_HASHTABLE_TEST_AMEN

typedef struct hashtable_data hashtable;
typedef struct hashtable_block block;

extern uint64_t AMEN;

// pairs: { key, value, key, value, AMEN }
uint64_t hash_mock(void* _pairs, uint64_t key);

#endif
