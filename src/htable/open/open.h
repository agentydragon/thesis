#ifndef HTABLE_OPEN_OPEN_H
#define HTABLE_OPEN_OPEN_H

#include <stdint.h>
#include <limits.h>

// Hash table with open addressing. Linear probing, simple tabulation hash.
// TODO: Components:
//	* TODO: Probing (linear, quadratic, double hashing)
//	* TODO: Hash function

#define HTOPEN_EMPTY_SLOT UINT64_MAX

typedef struct {
	uint64_t key;
	uint64_t value;
} htopen_slot;

typedef struct {
	uint32_t* keys_with_hash;
	// TODO: try storing keys separately from values?
	// (cache more useful that way)
	htopen_slot* table;
	uint64_t size;
	uint64_t capacity;
} htopen;

void htopen_init(htopen* this);
void htopen_destroy(htopen* this);
void htopen_find(htopen* this, uint64_t key);
int8_t htopen_insert(htopen* this, uint64_t key, uint64_t value);
int8_t htopen_delete(htopen* this, uint64_t key);

#endif
