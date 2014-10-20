#include "insert.h"
#include "../private/data.h"
#include "../private/find.h"

#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "../../log/log.h"

typedef struct hashbplustree tree;
typedef struct hashbplustree_node node;

static void assert_found(tree* tree, uint64_t key, uint64_t expected_value) {
	uint64_t value;
	bool found;
	if (hashbplustree_find(tree, key, &value, &found)) {
		log_fatal("failed to lookup key %" PRIu64 " "
				"(expecting value %" PRIu64 ")",
				key, expected_value);
	}
	if (!found) {
		log_fatal("didn't find key %" PRIu64 " "
				"(expecting value %" PRIu64 ")",
				key, expected_value);
	}
	if (value != expected_value) {
		log_fatal("found %" PRIu64 "=%" PRIu64 ", expecting =%" PRIu64,
				key, value, expected_value);
	}
}

static void assert_not_found(tree *tree, uint64_t key) {
	bool found;
	uint64_t value;
	if (hashbplustree_find(tree, key, &value, &found)) {
		log_fatal("failed to lookup key %" PRIu64 " "
				"(expecting not to find)", key);
	}
	if (found) {
		log_fatal("found %" PRIu64 "=%" PRIu64 " "
				"while not expecting to find anything",
				key, value);
	}
}

static void test_find_in_empty_tree() {
	node root = {
		.is_leaf = true,
		.keys_count = 0
	};
	tree tree = { .root = &root };
	assert_not_found(&tree, 42);
}

static void test_find_in_root_leaf() {
	node root = {
		.is_leaf = true,
		.keys_count = 3,
		.keys = { 1, 2, 3 },
		.values = { 11, 22, 33 }
	};
	tree tree = { .root = &root };
	assert_found(&tree, 1, 11);
	assert_found(&tree, 2, 22);
	assert_found(&tree, 3, 33);
	assert_not_found(&tree, 999);
}

static void test_deep_find() {
	node left_leaf = { // range: -infty ... 5
		.is_leaf = true,
		.keys_count = 3,
		.keys = { 1, 2, 3 },
		.values = { 10, 20, 30 }
	}, middle_leaf = {
		.is_leaf = true, // range: 15 ... 50
		.keys_count = 2,
		.keys = { 20, 21 },
		.values = { 200, 210 }
	}, right_leaf = {
		.is_leaf = true, // range: 80 ... infty
		.keys_count = 3,
		.keys = { 97, 98, 99 },
		.values = { 970, 980, 990 }
	};

	node fork = {
		.is_leaf = false,
		.keys_count = 3,
		.keys = { 5, 15, 50 },
		.pointers = { &left_leaf, NULL, &middle_leaf, NULL }
	};

	node root = {
		.is_leaf = false,
		.keys_count = 2,
		.keys = { 50, 80 },
		.pointers = { &fork, NULL, &right_leaf }
	};

	tree tree = { .root = &root };

	assert_found(&tree, 1, 10);
	assert_found(&tree, 2, 20);
	assert_found(&tree, 3, 30);
	assert_found(&tree, 20, 200);
	assert_found(&tree, 21, 210);
	assert_found(&tree, 97, 970);
	assert_found(&tree, 98, 980);
	assert_found(&tree, 99, 990);

	assert_not_found(&tree, 0); // left_leaf
	assert_not_found(&tree, 15); // middle_leaf
	assert_not_found(&tree, 22); // middle_leaf
	assert_not_found(&tree, 49); // middle_leaf
	assert_not_found(&tree, 100); // right_leaf

	// Other queries would hit NULL pointers.
}

void test_hash_bplustree_find() {
	test_find_in_empty_tree();
	test_find_in_root_leaf();
	test_deep_find();
}
