#include "../hash_bplustree/private/data.h"
#include "../hash_bplustree/private/insert.h"

#include <assert.h>

void test_inserting_to_empty_root_leaf() {
	struct hashbplustree_node root = {
		.is_leaf = true,
		.keys_count = 0
	};
	struct hashbplustree tree = {
		.root = &root
	};

	assert(!hashbplustree_insert(&tree, 10, 12345));

	assert(root.is_leaf);
	assert(root.keys_count == 1);
	assert(root.keys[0] == 10);
	assert(root.values[0] == 12345);
}

void test_inserting_to_nonempty_root_leaf() {
	struct hashbplustree_node root = {
		.is_leaf = true,
		.keys_count = 2,
		.keys = { 10, 30 },
		.values = { 100, 300 }
	};
	struct hashbplustree tree = {
		.root = &root
	};

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

void test_hash_bplustree() {
	test_inserting_to_empty_root_leaf();
	test_inserting_to_nonempty_root_leaf();
}
