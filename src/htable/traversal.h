#ifndef HTABLE_TRAVERSAL_H
#define HTABLE_TRAVERSAL_H

#include "htable/htable.h"

typedef struct {
	htable_block* block;
	uint8_t slot;
} htable_slot_pointer;

uint64_t htable_next_index(const htable* this, uint64_t i);

// TODO: docs
uint8_t htable_scan(htable* this, uint64_t key,
		htable_slot_pointer* key_slot,
		htable_slot_pointer* last_slot_with_hash,
		bool* found);

#endif
