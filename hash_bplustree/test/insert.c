#include "insert.h"
#include "helpers.h"
#include "../private/data.h"
#include "../private/insert.h"

#include <assert.h>
#include <inttypes.h>

#include "../../log/log.h"

static void test_inserting_to_empty_root_leaf() {
	node root = {
		.is_leaf = true,
		.keys_count = 0
	};
	tree tree = { .root = &root };

	assert(!hashbplustree_insert(&tree, 10, 12345));
	assert(root.is_leaf);
	assert_leaf(&root, {10, 12345});
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
		assert_leaf(&leaf, {1, 11}, {10, 100});
		assert_leaf(&split, {20, 200}, {30, 300});
	}

	{
		node leaf = {
			.keys_count = 3,
			.keys = { 10, 20, 30 },
			.values = { 100, 200, 300 }
		};
		hashbplustree_split_leaf_and_add(&leaf, 15, 150, &split);
		assert_leaf(&leaf, {10, 100}, {15, 150});
		assert_leaf(&split, {20, 200}, {30, 300});
	}

	{
		node leaf = {
			.keys_count = 3,
			.keys = { 10, 20, 30 },
			.values = { 100, 200, 300 }
		};
		hashbplustree_split_leaf_and_add(&leaf, 42, 420, &split);
		assert_leaf(&leaf, {10, 100}, {20, 200});
		assert_leaf(&split, {30, 300}, {42, 420});
	}
}

void test_splitting_internal() {
	node split;
	uint64_t split_key;
	void *A = (void*) 0x10, *B = (void*) 0x20, *C = (void*) 0x30,
		*D = (void*) 0x40, *X = (void*) 0xFFFF;

	{
		node target = {
			.keys_count = 3,
			.keys = { 10, 20, 30 },
			.pointers = { A, B, C, D }
		};
		hashbplustree_split_internal_and_add(&target, 5, X,
				&split, &split_key);

		assert_n_keys(&target, 2);
		assert_n_keys(&split, 1);

		assert_pointer(&target, 0, A);
		assert_key(&target, 0, 5);
		assert_pointer(&target, 1, X);
		assert_key(&target, 1, 10);
		assert_pointer(&target, 2, B);

		assert(split_key == 20);

		assert_pointer(&split, 0, C);
		assert_key(&split, 0, 30);
		assert_pointer(&split, 1, D);
	}

	{
		node target = {
			.keys_count = 3,
			.keys = { 10, 20, 30 },
			.pointers = { A, B, C, D }
		};
		hashbplustree_split_internal_and_add(&target, 35, X,
				&split, &split_key);

		assert_n_keys(&target, 2);
		assert_n_keys(&split, 1);

		assert_pointer(&target, 0, A);
		assert_key(&target, 0, 10);
		assert_pointer(&target, 1, B);
		assert_key(&target, 1, 20);
		assert_pointer(&target, 2, C);

		assert(split_key == 30);

		assert_pointer(&split, 0, D);
		assert_key(&split, 0, 35);
		assert_pointer(&split, 1, X);
	}
}

void test_splitting_root() {
	node original_root = {
		.keys_count = 3,
		.keys = { 10, 20, 30 },
		.values = { 100, 200, 300 },
		.is_leaf = true
	};
	tree tree = { .root = &original_root };
	hashbplustree_insert(&tree, 25, 250);

	assert(!tree.root->is_leaf);
	assert_key(tree.root, 0, 25);

	node* left = tree.root->pointers[0], *right = tree.root->pointers[1];
	assert_leaf(left, {10, 100}, {20, 200});
	assert_leaf(right, {25, 250}, {30, 300});
}

void test_hash_bplustree_insert() {
	test_inserting_to_empty_root_leaf();
	test_inserting_to_nonempty_root_leaf();
	test_splitting_leaves();
	test_splitting_internal();
	test_splitting_root();
}
