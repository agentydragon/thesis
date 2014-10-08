#include "insert.h"
#include "../private/data.h"
#include "../private/insert.h"

#include <assert.h>

typedef struct hashbplustree_node node;
typedef struct hashbplustree tree;

static void test_inserting_to_empty_root_leaf() {
	node root = {
		.is_leaf = true,
		.keys_count = 0
	};
	tree tree = { .root = &root };

	assert(!hashbplustree_insert(&tree, 10, 12345));

	assert(root.is_leaf);
	assert(root.keys_count == 1);
	assert(root.keys[0] == 10);
	assert(root.values[0] == 12345);
}

static void test_inserting_to_nonempty_root_leaf() {
	node root = {
		.is_leaf = true,
		.keys_count = 2,
		.keys = { 10, 30 },
		.values = { 100, 300 }
	};
	tree tree = { .root = &root };

	assert(!hashbplustree_insert(&tree, 20, 200));

	assert(root.is_leaf);
	assert(root.keys_count == 3);

	assert(root.keys[0] == 10);
	assert(root.values[0] == 100);

	assert(root.keys[1] == 20);
	assert(root.values[1] == 200);

	assert(root.keys[2] == 30);
	assert(root.values[2] == 300);
}

void test_splitting_leaves() {
	node split;

	{
		node leaf = {
			.keys_count = 3,
			.keys = { 10, 20, 30 },
			.values = { 100, 200, 300 }
		};
		hashbplustree_split_leaf_and_add(&leaf, 1, 11, &split);
		assert(leaf.keys_count == 2);
		assert(leaf.keys[0] == 1 && leaf.values[0] == 11);
		assert(leaf.keys[1] == 10 && leaf.values[1] == 100);

		assert(split.keys_count == 2);
		assert(split.keys[0] == 20 && split.values[0] == 200 &&
				split.keys[1] == 30 && split.values[1] == 300);
	}

	{
		node leaf = {
			.keys_count = 3,
			.keys = { 10, 20, 30 },
			.values = { 100, 200, 300 }
		};
		hashbplustree_split_leaf_and_add(&leaf, 15, 150, &split);
		assert(leaf.keys_count == 2);
		assert(leaf.keys[0] == 10 && leaf.values[0] == 100);
		assert(leaf.keys[1] == 15 && leaf.values[1] == 150);

		assert(split.keys_count == 2);
		assert(split.keys[0] == 20 && split.values[0] == 200 &&
				split.keys[1] == 30 && split.values[1] == 300);
	}

	{
		node leaf = {
			.keys_count = 3,
			.keys = { 10, 20, 30 },
			.values = { 100, 200, 300 }
		};
		hashbplustree_split_leaf_and_add(&leaf, 42, 420, &split);
		assert(leaf.keys_count == 2);
		assert(leaf.keys[0] == 10 && leaf.values[0] == 100);
		assert(leaf.keys[1] == 20 && leaf.values[1] == 200);

		assert(split.keys_count == 2);
		assert(split.keys[0] == 30 && split.values[0] == 300 &&
				split.keys[1] == 42 && split.values[1] == 420);
	}
}

void test_splitting_internal() {
	// node split;

	// TODO
	/*
	{
		node node = {
			.keys_count = 3
		};
	}
	*/
}

void test_hash_bplustree_insert() {
	test_inserting_to_empty_root_leaf();
	test_inserting_to_nonempty_root_leaf();
	test_splitting_leaves();
	test_splitting_internal();
}
