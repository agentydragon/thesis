#ifndef KFOREST_KFOREST_H
#define KFOREST_KFOREST_H

#include "btree/btree.h"
#include "rand/rand.h"

// We want the individual trees in the forest to be k-ary,
// so we are bound to B-trees.
// TODO: Can we genericize this?
#define KFOREST_K INTERNAL_MAX_KEYS

typedef struct {
	uint8_t tree_capacity;

	// The capacity of tree [i] is KFOREST_K^(2^i) - 1.
	// TODO: All B-trees but the last one are full, so we could probably
	// drop the pointers to get faster searches. That might mean some
	// more trouble with replacing, through.
	btree* trees;
	uint64_t* tree_sizes;

	rand_generator rng;
} kforest;

void kforest_init(kforest*);
void kforest_destroy(kforest*);

void kforest_find(kforest*, uint64_t key, uint64_t *value, bool *found);
int8_t kforest_insert(kforest*, uint64_t key, uint64_t value);
int8_t kforest_delete(kforest*, uint64_t key);

#endif
