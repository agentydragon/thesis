#include "htable/traversal.h"

#include <string.h>

#include "htable/hash.h"

uint64_t htable_next_index(const htable* this, uint64_t i) {
	return (i + 1) % this->blocks_size;
}
