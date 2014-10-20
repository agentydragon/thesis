#ifndef HASH_BPLUSTREE_PRIVATE_FIND_H
#define HASH_BPLUSTREE_PRIVATE_FIND_H

#include <stdint.h>
#include <stdbool.h>

int8_t hashbplustree_find(void* this, uint64_t key, uint64_t *value, bool *found);

#endif
