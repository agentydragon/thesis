#ifndef HASHTABLE_PRIVATE_INSERTION_H
#define HASHTABLE_PRIVATE_INSERTION_H

#include "helper.h"
#include <stdint.h>

int8_t hashtable_insert_internal(hashtable* this, uint64_t key, uint64_t value);

#endif
