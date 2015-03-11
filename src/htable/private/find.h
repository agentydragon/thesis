#ifndef HTABLE_PRIVATE_FIND_H
#define HTABLE_PRIVATE_FIND_H

#include <stdint.h>
#include <stdbool.h>

int8_t htable_find(void* _this, uint64_t key, uint64_t *value, bool *found);

#endif
