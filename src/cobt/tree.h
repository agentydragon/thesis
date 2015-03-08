#ifndef COBT_TREE_H
#define COBT_TREE_H

#include <stdint.h>

#include "veb_layout/veb_layout.h"

typedef struct {
	const uint64_t *backing_array;
	uint64_t backing_array_size;

	// The actual tree. Its size is hyperceil(size).
	uint64_t *tree;

	// Helper data for van Emde Boas layout navigation.
	struct level_data* level_data;
} cobt_tree;

typedef struct {
	// Represents an index range [begin;end)
	uint64_t begin;
	uint64_t end;
} cobt_tree_range;

void cobt_tree_init(cobt_tree*,
		const uint64_t* backing_array, uint64_t backing_array_size);
void cobt_tree_destroy(cobt_tree*);

// Finds the largest index I such that backing_array[I] <= key.
uint64_t cobt_tree_find_le(cobt_tree*, uint64_t key);

// Informs the tree about changes in a range
void cobt_tree_refresh(cobt_tree*, cobt_tree_range refresh);

#endif
