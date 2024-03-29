#ifndef COBT_H
#define COBT_H

// Cache-oblivious B-tree

#include <stdint.h>

#include "cobt/pma.h"
#include "cobt/tree.h"

#define COB_INFINITY UINT64_MAX

typedef struct {
	uint64_t key;
	uint64_t value;
} cob_piece_item;

#define COB_EMPTY UINT64_MAX

typedef struct {
	// The PMA stores groups of key-value pairs called "pieces".
	// They are keyed by their minimal keys.
	// Piece sizes are O(log N) multiples of 4.

	uint64_t size;  // Number of stored elements.
	uint8_t piece;  // Piece size in number of key-value pairs
	pma file;
	cobt_tree tree;
} cob;

void cob_init(cob* this);
void cob_destroy(cob* this);
bool cob_insert(cob* this, uint64_t key, uint64_t value);
bool cob_delete(cob* this, uint64_t key);
bool cob_find(cob* this, uint64_t key, uint64_t *value);
bool cob_next_key(cob* this, uint64_t key, uint64_t *next_key);
bool cob_previous_key(cob* this, uint64_t key, uint64_t *previous_key);
void cob_check(cob* this);

void cob_dump(const cob* this);
void cob_check_invariants(const cob* this);

#endif
