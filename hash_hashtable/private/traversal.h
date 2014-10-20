#ifndef HASH_HASHTABLE_PRIVATE_TRAVERSAL_H
#define HASH_HASHTABLE_PRIVATE_TRAVERSAL_H

#include "data.h"

struct hashtable_slot_pointer {
	struct hashtable_block* block;
	uint8_t slot;
};

uint64_t hashtable_next_index(struct hashtable_data* this, uint64_t i);
uint8_t hashtable_find_position(struct hashtable_data* this, uint64_t key,
		struct hashtable_slot_pointer* pointer, bool* found);

#endif
