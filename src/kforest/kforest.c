#include "kforest/kforest.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include "log/log.h"
#include "util/likeliness.h"

static int8_t expand(kforest* this) {
	uint64_t old_capacity = this->tree_capacity;
	uint64_t new_capacity = this->tree_capacity;

	if (new_capacity < 4) {
		new_capacity = 4;
	} else {
		new_capacity *= 2;
	}
	btree* new_trees = realloc(this->trees, sizeof(btree) * new_capacity);
	if (unlikely(!new_trees)) {
		log_error("failed to expand K-forest trees");
		return 1;
	}
	for (uint64_t i = old_capacity; i < new_capacity; ++i) {
		btree_init(&new_trees[i]);
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

static void check_invariants(kforest* this) {
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
		assert(this->tree_sizes[tree_count - 1] <= tree_capacity(tree_count - 1));
	}
	for (uint64_t i = tree_count; i < this->tree_capacity; ++i) {
		assert(this->tree_sizes[i] == 0);
	}
}
*/

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
		btree_destroy(&this->trees[i]);
	}
	free(this->trees);
	free(this->tree_sizes);
}

// This dropping algorithm would be probably biased in general, but since we
// are only dropping from full trees, it should be more or less OK.
// Implicit assumption: nodes have >1 key, key-value pairs are in leaves.
// Abuses the structure of the B-tree.
static void get_random_pair(rand_generator* generator, btree* tree,
		uint64_t *key, uint64_t *value) {
	// log_info("drop random from btree %p", tree);

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

static void drop_random_pair(rand_generator* generator, btree* tree,
		uint64_t *dropped_key, uint64_t *dropped_value) {
	get_random_pair(generator, tree, dropped_key, dropped_value);
	ASSERT(btree_delete(tree, *dropped_key) == 0);
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

		ASSERT(btree_insert(&this->trees[tree + 1], key, value) == 0);
		++this->tree_sizes[tree + 1];

		//max_touched = tree + 2;
	}
}

void kforest_find(kforest* this, uint64_t key, uint64_t *value, bool *found) {
	log_verbose(1, "kforest_find(%" PRIu64 ")", key);

	uint64_t tree;
	uint64_t value_found;
	for (tree = 0; tree < this->tree_capacity; ++tree) {
		bool found_here;
		btree_find(&this->trees[tree], key, &value_found, &found_here);
		if (found_here) {
			goto tree_found;
		}
	}
	goto not_found;

tree_found:
	*found = true;
	if (value) {
		*value = value_found;
	}

	ASSERT(btree_delete(&this->trees[tree], key) == 0);
	--this->tree_sizes[tree];

	make_space(this);

	// Promote to tree 0.
	ASSERT(btree_insert(&this->trees[0], key, value_found) == 0);
	++this->tree_sizes[0];

	//dump(this);
	//check_invariants(this);
	return;

not_found:
	//dump(this);
	//check_invariants(this);
	*found = false;
}

int8_t kforest_insert(kforest* this, uint64_t key, uint64_t value) {
	log_verbose(1, "kforest_insert(%" PRIu64 "=%" PRIu64 ")", key, value);

	bool exists;
	kforest_find(this, key, NULL, &exists);
	if (exists) {
		return 1;
	}

	make_space(this);

	ASSERT(btree_insert(&this->trees[0], key, value) == 0);
	++this->tree_sizes[0];

	//btree_dump_dot(&this->trees[0], stdout);

	//dump(this);
	//check_invariants(this);
	//log_info("// kforest_insert");

	return 0;
}

int8_t kforest_delete(kforest* this, uint64_t key) {
	log_verbose(1, "kforest_delete(%" PRIu64 ")", key);

	bool found;
	kforest_find(this, key, NULL, &found);
	if (!found) {
		return 1;
	}

	ASSERT(btree_delete(&this->trees[0], key) == 0);
	--this->tree_sizes[0];

	for (uint64_t tree = 0; tree + 1 < this->tree_capacity; ++tree) {
		if (this->tree_sizes[tree + 1] > 0) {
			uint64_t shift_key, shift_value;
			drop_random_pair(&this->rng, &this->trees[tree + 1],
					&shift_key, &shift_value);
			--this->tree_sizes[tree + 1];
			ASSERT(btree_insert(&this->trees[tree], shift_key,
						shift_value) == 0);
			++this->tree_sizes[tree];
		}
	}

	//dump(this);
	//check_invariants(this);

	return 0;
}
