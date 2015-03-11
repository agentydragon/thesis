#ifndef HTABLE_PRIVATE_TRAVERSAL_H
#define HTABLE_PRIVATE_TRAVERSAL_H

#include "htable/htable.h"
#include "htable/private/helper.h"

struct htable_slot_pointer {
	block* block;
	uint8_t slot;
};

uint64_t htable_next_index(const htable* this, uint64_t i);

// TODO: docs
uint8_t htable_scan(htable* this, uint64_t key,
		slot_pointer* key_slot, slot_pointer* last_slot_with_hash,
		bool* found);

#endif
