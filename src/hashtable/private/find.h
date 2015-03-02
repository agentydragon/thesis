#ifndef HASHTABLE_PRIVATE_FIND_H
#define HASHTABLE_PRIVATE_FIND_H

#include <stdint.h>
#include <stdbool.h>

int8_t hashtable_find(void* _this, uint64_t key, uint64_t *value, bool *found);

#endif
