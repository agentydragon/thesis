#ifndef HASH_HASHTABLE_PRIVATE_INSERTION_H
#define HASH_HASHTABLE_PRIVATE_INSERTION_H

#include <stdint.h>

struct hashtable_data;
int8_t hashtable_insert_internal(struct hashtable_data* this, uint64_t key, uint64_t value);

#endif
