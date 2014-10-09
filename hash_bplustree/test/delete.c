#include "delete.h"
#include "helpers.h"
#include "../private/data.h"
#include "../private/delete.h"

#include <assert.h>
#include <inttypes.h>

#include "../../log/log.h"

static void test_deleting_from_root_leaf() {
	node root = {
		.is_leaf = true,
		.keys_count = 3,
		.keys = { 10, 20, 30 },
		.values = { 11, 22, 33 }
	};
	tree tree = { .root = &root };

	assert(hashbplustree_delete(&tree, 999));

	assert(!hashbplustree_delete(&tree, 20));
	assert_leaf(&root, {10, 11}, {30, 33});

	assert(!hashbplustree_delete(&tree, 10));
	assert_leaf(&root, {30, 33});

	assert(!hashbplustree_delete(&tree, 30));
	assert_empty_leaf(&root);

	assert(hashbplustree_delete(&tree, 999));
}

static void test_collapsing_into_root_leaf() {
	node *a, *b, *root;
	a = malloc(sizeof(node));
	b = malloc(sizeof(node));
	root = malloc(sizeof(node));

	*a = (node) {
		.is_leaf = true,
		.keys_count = 2,
		.keys = { 1, 2 },
		.values = { 11, 22 }
	};
	*b = (node) {
		.is_leaf = true,
		.keys_count = 2,
		.keys = { 100 , 200 },
		.values = { 111, 222 }
	};
	*root = (node) {
		.is_leaf = false,
		.keys_count = 1,
		.keys = { 100 },
		.pointers = { a, b }
	};
	tree tree = { .root = root };

	assert(!hashbplustree_delete(&tree, 100));

	// Left got all nodes. Right got deleted, left got promoted to root.
	// Root got deleted. Just left is still allocated.
	assert(tree.root == a);

	assert(tree.root->is_leaf);
	assert_leaf(tree.root, {1, 11}, {2, 22}, {200, 222});

	free(tree.root);
}

void test_merging_internal_nodes() {
	void* A = (void*) 0x10, *B = (void*) 0x20, *C = (void*) 0x30,
		*D = (void*) 0x40;

	{
		node a = (node) {
			.is_leaf = false,
			.keys_count = 1,
			.keys = { 10 },
			.pointers = { A, B }
		}, b = (node) {
			.is_leaf = false,
			.keys_count = 1,
			.keys = { 30 },
			.pointers = { C, D }
		};

		bplustree_merge_internal_nodes(&a, &b, 20, true);
		assert_n_keys(&a, 3);
		assert_pointer(&a, 0, A);
		assert_key(&a, 0, 10);
		assert_pointer(&a, 1, B);
		assert_key(&a, 1, 20);
		assert_pointer(&a, 2, C);
		assert_key(&a, 2, 30);
		assert_pointer(&a, 3, D);

		assert_n_keys(&b, 0);
	}

	{
		node a = (node) {
			.is_leaf = false,
			.keys_count = 1,
			.keys = { 10 },
			.pointers = { A, B }
		}, b = (node) {
			.is_leaf = false,
			.keys_count = 1,
			.keys = { 30 },
			.pointers = { C, D }
		};

		bplustree_merge_internal_nodes(&a, &b, 20, false);
		assert_n_keys(&b, 3);
		assert_pointer(&b, 0, A);
		assert_key(&b, 0, 10);
		assert_pointer(&b, 1, B);
		assert_key(&b, 1, 20);
		assert_pointer(&b, 2, C);
		assert_key(&b, 2, 30);
		assert_pointer(&b, 3, D);

		assert_n_keys(&a, 0);
	}
}

void test_hash_bplustree_delete() {
	test_deleting_from_root_leaf();
	test_collapsing_into_root_leaf();
	test_merging_internal_nodes();
	// TODO
}
