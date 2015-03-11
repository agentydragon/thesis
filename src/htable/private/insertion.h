#ifndef HTABLE_PRIVATE_INSERTION_H
#define HTABLE_PRIVATE_INSERTION_H

#include "helper.h"
#include <stdint.h>

int8_t htable_insert_internal(htable* this, uint64_t key, uint64_t value);

#endif
