#ifndef HTABLE_RESIZE_H
#define HTABLE_RESIZE_H

#include "htable/htable.h"

int8_t htable_resize_to_fit(htable* this, uint64_t new_size);

#endif
