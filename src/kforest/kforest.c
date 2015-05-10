#include "kforest/kforest.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "log/log.h"
#include "util/likeliness.h"

#if defined(KFOREST_BTREE)
	#define kforest_tree_init btree_init
	#define kforest_tree_destroy btree_destroy
	#define kforest_tree_delete btree_delete
	#define kforest_tree_insert btree_insert
	#define kforest_tree_find btree_find

	// This dropping algorithm would be probably biased in general, but
	// since we are only dropping from full trees, it should be more or
	// less OK.
	// Implicit assumption: nodes have >1 key, pairs are in leaves.
	static void get_random_pair(rand_generator* generator, btree* tree,
			uint64_t *key, uint64_t *value) {
		// log_info("drop random from tree %p", tree);

		btree_node_traversed node = nt_root(tree);
		// log_info("  root=%p", node.persisted);
		while (!nt_is_leaf(node)) {
			uint64_t child_index = rand_next(generator,
					node.persisted->internal.key_count + 1);
			node.persisted = node.persisted->internal.pointers[child_index];
			// log_info("  ->child %" PRIu64 ": %p",
			// 		child_index, node.persisted);
			--node.levels_above_leaves;
		}

		const uint8_t leaf_keys = get_n_leaf_keys(node.persisted);
		assert(leaf_keys > 0);
		uint64_t key_index = rand_next(generator, leaf_keys);
		*key = node.persisted->leaf.keys[key_index];
		*value = node.persisted->leaf.values[key_index];
	}
#elif defined(KFOREST_COBT)
	#define kforest_tree_init cob_init
	#define kforest_tree_destroy cob_destroy
	#define kforest_tree_delete cob_delete
	#define kforest_tree_insert cob_insert
	#define kforest_tree_find cob_find

	// Point to random spot in the backing PMA, then find the next occupied
	// piece. Select a random key from that piece.
	// The selected key should be more or less random.
	static void get_random_pair(rand_generator* generator, cob* tree,
			uint64_t *key, uint64_t *value) {
		uint64_t piece_pick = rand_next(generator, tree->file.capacity);
		uint64_t i;
		for (i = 0; i < tree->file.capacity; ++i) {
			if (tree->file.occupied[(piece_pick + i) %
					tree->file.capacity]) {
				break;
			}
		}
		CHECK(i < tree->file.capacity, "random pick from empty cobt");
		piece_pick = (piece_pick + i) % tree->file.capacity;
		cob_piece_item* piece = pma_get_value(&tree->file, piece_pick);

		uint8_t idx = rand_next(generator, tree->piece);
		for (i = 0; i < tree->piece; ++i) {
			if (piece[(idx + i) % tree->piece].key != COB_EMPTY) {
				*key = piece[(idx + i) % tree->piece].key;
				*value = piece[(idx + i) % tree->piece].value;
				return;
			}
		}
		log_fatal("fetched empty piece from pma");
	}
#else
	#error "No K-forest backing structure selected."
#endif

static int8_t expand(kforest* this) {
	uint64_t old_capacity = this->tree_capacity;
	uint64_t new_capacity = this->tree_capacity;

	if (new_capacity < 4) {
		new_capacity = 4;
	} else {
		new_capacity *= 2;
	}
	kforest_tree* new_trees = realloc(this->trees,
			sizeof(kforest_tree) * new_capacity);
	if (unlikely(!new_trees)) {
		log_error("failed to expand K-forest trees");
		return 1;
	}
	for (uint64_t i = old_capacity; i < new_capacity; ++i) {
		// TODO: unify zeroedness requirements
		memset(&new_trees[i], 0, sizeof(kforest_tree));
		kforest_tree_init(&new_trees[i]);
	}
	this->trees = new_trees;

	uint64_t* new_tree_sizes = realloc(this->tree_sizes, sizeof(uint64_t) *
			new_capacity);
	if (unlikely(!new_tree_sizes)) {
		log_error("failed to expand K-forest tree sizes");
		return 1;
	}
	for (uint64_t i = old_capacity; i < new_capacity; ++i) {
		new_tree_sizes[i] = 0;
	}
	this->tree_sizes = new_tree_sizes;
	this->tree_capacity = new_capacity;
	return 0;
}

// Tree count examples:
//     i=0 -> tree size 4
//     i=1 -> tree size 4^2  =            16
//     i=2 -> tree size 4^4  =           256
//     i=3 -> tree size 4^8  =        65 536
//     i=4 -> tree size 4^16 = 4 294 967 296
static uint64_t tree_capacity(uint64_t tree_index) {
	uint64_t base = 1;
	for (uint64_t i = 0; i < (1ULL << tree_index); ++i) {
		base *= KFOREST_K;
	}
	return base - 1;
}

/*
static void dump_tree(kforest* this, uint64_t index) {
	log_info("[%" PRIu64 "] cap=%" PRIu64 " size=%" PRIu64,
			index, tree_capacity(index), this->tree_sizes[index]);
	btree_dump_dot(&this->trees[index], stdout);
}

static void dump(kforest* this) {
	for (uint64_t i = 0; i < this->tree_capacity; ++i) {
		dump_tree(this, i);
	}
}
*/

void kforest_check_invariants(kforest* this) {
	uint64_t tree_count = 0;
	for (uint64_t i = 0; i < this->tree_capacity; ++i) {
		if (this->tree_sizes[i] > 0) {
			++tree_count;
		}
	}

	for (uint64_t i = 0; i + 1 < tree_count; ++i) {
		assert(this->tree_sizes[i] == tree_capacity(i));
	}
	if (tree_count > 0) {
		assert(this->tree_sizes[tree_count - 1] <=
				tree_capacity(tree_count - 1));
	}
	for (uint64_t i = tree_count; i < this->tree_capacity; ++i) {
		assert(this->tree_sizes[i] == 0);
	}
}

int8_t kforest_init(kforest* this) {
	this->tree_capacity = 0;
	this->trees = NULL;
	this->tree_sizes = NULL;
	if (unlikely(expand(this))) {
		free(this->trees);
		free(this->tree_sizes);
		return 1;
	}

	// Seed RNG.
	this->rng.state = 42;
	return 0;
}

void kforest_destroy(kforest* this) {
	for (uint64_t i = 0; i < this->tree_capacity; ++i) {
		kforest_tree_destroy(&this->trees[i]);
	}
	free(this->trees);
	free(this->tree_sizes);
}

static void drop_random_pair(rand_generator* generator, kforest_tree* tree,
		uint64_t *dropped_key, uint64_t *dropped_value) {
	get_random_pair(generator, tree, dropped_key, dropped_value);
	ASSERT(kforest_tree_delete(tree, *dropped_key));
}

static void make_space(kforest* this) {
	//uint64_t max_touched = 0;
	for (uint64_t tree = 0;
			(tree == 0 && this->tree_sizes[0] > tree_capacity(0) - 1) ||
					this->tree_sizes[tree] > tree_capacity(tree);
			++tree) {
		while (tree + 1 >= this->tree_capacity) {
			ASSERT(expand(this) == 0);
		}
		uint64_t key, value;
		assert(this->tree_sizes[tree] > 0);
		drop_random_pair(&this->rng, &this->trees[tree], &key, &value);
		--this->tree_sizes[tree];

		log_verbose(2, "moving %" PRIu64 "=%" PRIu64 " "
				"from tree %" PRIu64 " to tree %" PRIu64,
				key, value, tree, tree + 1);

		ASSERT(kforest_tree_insert(&this->trees[tree + 1], key, value));
		++this->tree_sizes[tree + 1];

		//max_touched = tree + 2;
	}
}

bool kforest_find(kforest* this, uint64_t key, uint64_t *value) {
	log_verbose(1, "kforest_find(%" PRIu64 ")", key);

	uint64_t tree;
	uint64_t value_found;
	for (tree = 0; tree < this->tree_capacity; ++tree) {
		if (kforest_tree_find(&this->trees[tree], key, &value_found)) {
			goto tree_found;
		}
	}
	// Not found.
	return false;

tree_found:
	if (value) {
		*value = value_found;
	}

	// If we found the key in the first tree, there's no need to drop keys.
	if (tree != 0) {
		ASSERT(kforest_tree_delete(&this->trees[tree], key));
		--this->tree_sizes[tree];

		make_space(this);

		// Promote to tree 0.
		ASSERT(kforest_tree_insert(&this->trees[0], key, value_found));
		++this->tree_sizes[0];
	}
	return true;
}

bool kforest_insert(kforest* this, uint64_t key, uint64_t value) {
	log_verbose(1, "kforest_insert(%" PRIu64 "=%" PRIu64 ")", key, value);

	if (kforest_find(this, key, NULL)) {
		return false;
	}

	make_space(this);

	ASSERT(kforest_tree_insert(&this->trees[0], key, value));
	++this->tree_sizes[0];

	//btree_dump_dot(&this->trees[0], stdout);

	//dump(this);
	//check_invariants(this);

	return true;
}

bool kforest_delete(kforest* this, uint64_t key) {
	log_verbose(1, "kforest_delete(%" PRIu64 ")", key);

	// TODO: This is slightly slow. Deleting should not promote first.
	if (!kforest_find(this, key, NULL)) {
		return false;
	}

	ASSERT(kforest_tree_delete(&this->trees[0], key));
	--this->tree_sizes[0];

	for (uint64_t tree = 0; tree + 1 < this->tree_capacity; ++tree) {
		if (this->tree_sizes[tree + 1] > 0) {
			uint64_t shift_key, shift_value;
			drop_random_pair(&this->rng, &this->trees[tree + 1],
					&shift_key, &shift_value);
			--this->tree_sizes[tree + 1];
			ASSERT(kforest_tree_insert(&this->trees[tree],
						shift_key, shift_value));
			++this->tree_sizes[tree];
		}
	}

	//dump(this);
	//check_invariants(this);

	return true;
}
