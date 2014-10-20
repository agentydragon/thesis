#include "delete.h"
#include "helpers.h"
#include "../private/data.h"
#include "../private/delete.h"
#include "../private/dump.h"

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

	assert(tree.root == root);
	assert(root->is_leaf);
	assert_leaf(root, {1, 11}, {2, 22}, {200, 222});

	free(root);
}

void test_collapse_level() {
	void* A = (void*) 0x10, *B = (void*) 0x20;
	node *root, *collapsed_left, *leaf1, *leaf2, *right;
	root = malloc(sizeof(node));
	collapsed_left = malloc(sizeof(node));
	leaf1 = malloc(sizeof(node));
	leaf2 = malloc(sizeof(node));
	right = malloc(sizeof(node));

	*root = (node) {
		.is_leaf = false,
		.keys_count = 1,
		.keys = { 1000 },
		.pointers = { collapsed_left, right }
	};

	*collapsed_left = (node) {
		.is_leaf = false,
		.keys_count = 1,
		.keys = { 100 },
		.pointers = { leaf1, leaf2 }
	};

	*leaf1 = (node) {
		.is_leaf = true,
		.keys_count = 2,
		.keys = { 1, 2 },
		.values = { 10, 20 }
	};

	*leaf2 = (node) {
		.is_leaf = true,
		.keys_count = 2,
		.keys = { 150, 160 },
		.values = { 151, 161 }
	};

	*right = (node) {
		.is_leaf = false,
		.keys_count = 1,
		.keys = { 2000 },
		.pointers = { A, B }
	};

	tree tree = { .root = root };

	assert(!hashbplustree_delete(&tree, 2));

	assert(tree.root == root);
	assert_n_keys(root, 1);
	assert_pointer(root, 0, collapsed_left);
	assert_key(root, 0, 1000);
	assert_pointer(root, 1, right);

	assert_leaf(collapsed_left, {1, 10}, {150, 151}, {160, 161});

	free(root);
	free(collapsed_left);
	free(right);
}

void test_hash_bplustree_delete() {
	test_deleting_from_root_leaf();
	test_collapsing_into_root_leaf();
	test_collapse_level();
	// TODO
}
