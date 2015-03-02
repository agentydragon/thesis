#ifndef HASHTABLE_TEST_HELPER_H
#define HASHTABLE_TEST_HELPER_H

#include "hashtable/private/data.h"

#define hash_mock hashtable_test_hash_mock
#define AMEN HASHTABLE_TEST_AMEN

typedef struct hashtable_data hashtable;
typedef struct hashtable_block block;

extern uint64_t AMEN;

// pairs: { key, value, key, value, AMEN }
uint64_t hash_mock(void* _pairs, uint64_t key);

#endif
