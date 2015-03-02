#ifndef HASHTABLE_PRIVATE_TRAVERSAL_H
#define HASHTABLE_PRIVATE_TRAVERSAL_H

#include "data.h"
#include "helper.h"

struct hashtable_slot_pointer {
	block* block;
	uint8_t slot;
};

uint64_t hashtable_next_index(const hashtable* this, uint64_t i);

// TODO: docs
uint8_t hashtable_scan(hashtable* this, uint64_t key,
		slot_pointer* key_slot, slot_pointer* last_slot_with_hash,
		bool* found);

#endif
