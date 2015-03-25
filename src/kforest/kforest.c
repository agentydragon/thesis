#include "kforest/kforest.h"

#include <assert.h>
#include <stdlib.h>

static void expand(kforest* this) {
	uint64_t old_capacity = this->tree_capacity;

	if (this->tree_capacity < 4) {
		this->tree_capacity = 4;
	} else {
		this->tree_capacity *= 2;
	}
	this->trees = realloc(this->trees, sizeof(btree) * this->tree_capacity);
	this->tree_sizes = realloc(this->tree_sizes, sizeof(uint64_t) *
			this->tree_capacity);

	for (uint64_t i = old_capacity; i < this->tree_capacity; ++i) {
		btree_init(&this->trees[i]);
		this->tree_sizes[i] = 0;
	}
}

static uint64_t tree_capacity(uint64_t tree_index) {
	uint64_t base = 1;
	for (uint64_t i = 0; i < tree_index; ++i) {
		base *= KFOREST_K;
	}
	return base * (1 << tree_index) - 1;
}

void kforest_init(kforest* this) {
	this->tree_capacity = 0;
	this->trees = NULL;
	this->tree_sizes = NULL;
	expand(this);

	// Seed RNG.
	this->rng.state = 42;
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
	btree_node_traversed node;
	for (node = nt_root(tree); !nt_is_leaf(node); ) {
		uint64_t child_index = rand_next(generator,
				node.persisted->internal.key_count) + 1;
		node.persisted = node.persisted->internal.pointers[child_index];
		--node.levels_above_leaves;
	}

	uint64_t key_index = rand_next(generator,
			get_n_leaf_keys(node.persisted));
	*key = node.persisted->leaf.keys[key_index];
	*value = node.persisted->leaf.values[key_index];
}

static void drop_random_pair(rand_generator* generator, btree* tree,
		uint64_t *dropped_key, uint64_t *dropped_value) {
	get_random_pair(generator, tree, dropped_key, dropped_value);
	assert(btree_delete(tree, *dropped_key) == 0);
}

static void make_space(kforest* this) {
	for (uint64_t tree = 0;
			(tree == 0 && this->tree_sizes[tree] > tree_capacity(tree) - 1) ||
					this->tree_sizes[tree] > tree_capacity(tree);
			++tree) {
		assert(this->tree_sizes[tree] > 0);
		while (tree + 1 >= this->tree_capacity) {
			expand(this);
		}
		uint64_t key, value;
		drop_random_pair(&this->rng, &this->trees[tree], &key, &value);
		--this->tree_sizes[tree];

		assert(btree_insert(&this->trees[tree + 1], key, value));
		++this->tree_sizes[tree + 1];
	}
}

void kforest_find(kforest* this, uint64_t key, uint64_t *value, bool *found) {
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

	assert(btree_delete(&this->trees[tree], key) == 0);
	--this->tree_sizes[tree];

	make_space(this);

	// Promote to tree 0.
	assert(btree_insert(&this->trees[0], key, value_found) == 0);
	++this->tree_sizes[0];

	return;

not_found:
	*found = false;
}

int8_t kforest_insert(kforest* this, uint64_t key, uint64_t value) {
	bool exists;
	kforest_find(this, key, NULL, &exists);
	if (exists) {
		return 1;
	}

	make_space(this);

	assert(btree_insert(&this->trees[0], key, value) == 0);
	++this->tree_sizes[0];

	return 0;
}

int8_t kforest_delete(kforest* this, uint64_t key) {
	uint64_t tree;
	bool found = false;
	for (tree = 0; tree < this->tree_capacity && !found; ++tree) {
		btree_find(&this->trees[tree], key, NULL, &found);
	}

	if (!found) {
		return 1;
	}

	assert(btree_delete(&this->trees[tree], key) == 0);
	--this->tree_sizes[0];

	for (; tree + 1 < this->tree_capacity; ++tree) {
		if (this->tree_sizes[tree + 1] > 0) {
			uint64_t shift_key, shift_value;
			drop_random_pair(&this->rng, &this->trees[tree + 1],
					&shift_key, &shift_value);
			assert(btree_insert(&this->trees[tree], shift_key,
						shift_value) == 0);
		}
	}

	return 0;
}
