#ifndef HASH_HASHTABLE_PRIVATE_RESIZING_H
#define HASH_HASHTABLE_PRIVATE_RESIZING_H

#include "data.h"

int8_t hashtable_resize(struct hashtable_data* this, uint64_t new_size);

#endif
