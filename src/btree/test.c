#include "btree.h"
#include "../log/log.h"

#include <assert.h>

#define MOCK(x) ((void*) (x))

static void test_internal_splitting() {
	btree_node_persisted node = {
		.internal = {
			.key_count = 3,
		},
		.keys = { 100, 200, 300 },
		.pointers = { MOCK(50), MOCK(150), MOCK(250), MOCK(350) }
	};
	btree_node_persisted new_right_sibling;
	uint64_t middle;

	split_internal(&node, &new_right_sibling, &middle);

	assert(node.internal.key_count == 1);
	assert(node.keys[0] == 100);
	assert(node.pointers[0] == MOCK(50) && node.pointers[1] == MOCK(150));

	assert(middle == 200);

	assert(new_right_sibling.internal.key_count == 1);
	assert(new_right_sibling.keys[0] == 300);
	assert(new_right_sibling.pointers[0] == MOCK(250) &&
			new_right_sibling.pointers[1] == MOCK(350));
	log_info("internal splitting ok");
}

static void test_leaf_splitting() {
	btree_node_persisted node = {
		.leaf = {
			.key_count = 3,
		},
		.keys = { 10, 20, 30 },
		.values = { 111, 222, 333 }
	};
	btree_node_persisted new_right_sibling;
	uint64_t middle;

	split_leaf(&node, &new_right_sibling, &middle);

	assert(node.leaf.key_count == 1);
	assert(node.keys[0] == 10);
	assert(node.values[0] == 111);

	assert(middle == 20);

	assert(new_right_sibling.leaf.key_count == 2);
	assert(new_right_sibling.keys[0] == 20 &&
			new_right_sibling.keys[1] == 30);
	assert(new_right_sibling.values[0] == 222 &&
			new_right_sibling.values[1] == 333);
	log_info("leaf splitting ok");
}

static void test_insert_pointer() {
	btree_node_persisted node = {
		.internal = {
			.key_count = 2,
		},
		.keys = { 100, 300 },
		.pointers = { MOCK(50), MOCK(150), MOCK(350) }
	};
	insert_pointer(&node, 200, MOCK(250));
	assert(node.internal.key_count == 3);
	assert(node.keys[0] = 100 && node.keys[1] == 200 &&
			node.keys[2] == 300);
	assert(node.pointers[0] == MOCK(50) && node.pointers[1] == MOCK(150) &&
			node.pointers[2] == MOCK(250) &&
			node.pointers[3] == MOCK(350));
}

static void test_insert_key_value_pair() {
	btree_node_persisted node = {
		.leaf = {
			.key_count = 2,
		},
		.keys = { 100, 300 },
		.values = { 11, 33 }
	};
	insert_key_value_pair(&node, 200, 22);
	assert(node.leaf.key_count == 3);
	assert(node.keys[0] = 100 && node.keys[1] == 200 &&
			node.keys[2] == 300);
	assert(node.values[0] == 11 && node.values[1] == 22 &&
			node.values[2] == 33);
}

static void test_inserting() {
	btree tree;
	btree_init(&tree);
	btree_insert(&tree, 10, 100);
	btree_insert(&tree, 5, 50);
	btree_insert(&tree, 15, 150);
	btree_insert(&tree, 14, 140);
	btree_insert(&tree, 21, 210);
	btree_insert(&tree, 17, 170);
	btree_insert(&tree, 3, 30);
	btree_insert(&tree, 9, 90);
	btree_insert(&tree, 4, 40);
	btree_insert(&tree, 1, 10);
	btree_insert(&tree, 16, 160);
	btree_insert(&tree, 13, 130);
	btree_insert(&tree, 20, 200);
	btree_insert(&tree, 22, 220);
	btree_insert(&tree, 23, 230);
	btree_insert(&tree, 28, 280);
	btree_insert(&tree, 11, 110);
	btree_insert(&tree, 2, 20);
	btree_insert(&tree, 6, 60);
	btree_insert(&tree, 7, 70);
	btree_dump(&tree);
	btree_destroy(&tree);
}

static void test_deletion() {
	btree tree;
	btree_init(&tree);
	btree_insert(&tree, 10, 100);
	btree_insert(&tree, 5, 50);
	btree_insert(&tree, 15, 150);
	btree_insert(&tree, 14, 140);
	btree_insert(&tree, 21, 210);
	btree_insert(&tree, 17, 170);
	btree_insert(&tree, 3, 30);
	btree_insert(&tree, 9, 90);
	btree_insert(&tree, 4, 40);
	btree_insert(&tree, 1, 10);
	btree_insert(&tree, 16, 160);
	btree_insert(&tree, 13, 130);
	btree_insert(&tree, 20, 200);
	btree_insert(&tree, 22, 220);
	btree_insert(&tree, 23, 230);
	btree_insert(&tree, 28, 280);
	btree_insert(&tree, 11, 110);
	btree_insert(&tree, 2, 20);
	btree_insert(&tree, 6, 60);
	btree_insert(&tree, 7, 70);

	btree_delete(&tree, 14);
	btree_delete(&tree, 15);
	btree_delete(&tree, 1);
	btree_delete(&tree, 2);
	btree_delete(&tree, 4);
	btree_delete(&tree, 5);
	btree_delete(&tree, 28);
	btree_delete(&tree, 22);
	btree_delete(&tree, 16);
	btree_delete(&tree, 11);
	btree_delete(&tree, 10);
	btree_delete(&tree, 6);
	btree_delete(&tree, 9);
	btree_delete(&tree, 17);
	btree_delete(&tree, 13);
	btree_delete(&tree, 23);
	btree_delete(&tree, 7);
	btree_delete(&tree, 20);
	btree_delete(&tree, 3);
	btree_delete(&tree, 21);
	btree_destroy(&tree);
}

void test_btree() {
	test_internal_splitting();
	test_leaf_splitting();
	test_insert_pointer();
	test_insert_key_value_pair();
	test_inserting();
	test_deletion();
}
