#ifndef HTABLE_TEST_HELPER_H
#define HTABLE_TEST_HELPER_H

#include "htable/private/data.h"

#define hash_mock htable_test_hash_mock
#define AMEN HTABLE_TEST_AMEN

typedef struct htable_data htable;
typedef struct htable_block block;

extern uint64_t AMEN;

// pairs: { key, value, key, value, AMEN }
uint64_t hash_mock(void* _pairs, uint64_t key);

#endif
