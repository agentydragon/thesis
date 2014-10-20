#ifndef HASH_HASHTABLE_PRIVATE_RESIZING_H
#define HASH_HASHTABLE_PRIVATE_RESIZING_H

#include "helper.h"
#include <stdint.h>

int8_t hashtable_resize_to_fit(hashtable* this, uint64_t new_size);

#endif
