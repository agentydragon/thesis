#ifndef HASH_HASHTABLE_PRIVATE_TRAVERSAL_H
#define HASH_HASHTABLE_PRIVATE_TRAVERSAL_H

#include "data.h"
#include "helper.h"

struct hashtable_slot_pointer {
	struct hashtable_block* block;
	uint8_t slot;
};

uint64_t hashtable_next_index(struct hashtable_data* this, uint64_t i);

// TODO: docs
uint8_t hashtable_scan(
		struct hashtable_data* this, uint64_t key,
		slot_pointer* key_slot, slot_pointer* last_slot_with_hash,
		bool* found);

#endif
