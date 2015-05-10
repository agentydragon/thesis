#ifndef KFOREST_KFOREST_H
#define KFOREST_KFOREST_H

#include "rand/rand.h"

#define KFOREST_BTREE
// #define KFOREST_COBT

#if defined(KFOREST_BTREE)
	#include "btree/btree.h"
	typedef btree kforest_tree;
	// TODO: Can we genericize this?
	#define KFOREST_K LEAF_MAX_KEYS
#elif defined(KFOREST_COBT)
	#include "cobt/cobt.h"
	typedef cob kforest_tree;
	// Arbitrary pick.
	#define KFOREST_K 5
#else
	#error "No K-forest backing structure selected."
#endif

// TODO: Optimize - don't delete when match is in tree 1.

typedef struct {
	uint8_t tree_capacity;

	// The capacity of tree [i] is KFOREST_K^(2^i) - 1.
	// TODO: All B-trees but the last one are full, so we could probably
	// drop the pointers to get faster searches. That might mean some
	// more trouble with replacing, through.
	kforest_tree* trees;
	uint64_t* tree_sizes;

	rand_generator rng;
} kforest;

// TODO: "must_use_result"
int8_t kforest_init(kforest*);
void kforest_destroy(kforest*);

bool kforest_find(kforest*, uint64_t key, uint64_t *value);
bool kforest_insert(kforest*, uint64_t key, uint64_t value);
bool kforest_delete(kforest*, uint64_t key);

void kforest_check_invariants(kforest*);

#endif
