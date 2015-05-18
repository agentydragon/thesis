#include "btree/btree.h"
#include "log/log.h"

#include <assert.h>

#define MOCK(x) ((void*) (x))

static void test_internal_splitting(void) {
	btree_node_persisted node = {
		.internal = {
			.key_count = 3,
			.keys = { 100, 200, 300 },
			.pointers = { MOCK(50), MOCK(150), MOCK(250), MOCK(350) }
		},
	};
	btree_node_persisted new_right_sibling;
	uint64_t middle;

	split_internal(&node, &new_right_sibling, &middle);

	assert(node.internal.key_count == 1);
	assert(node.internal.keys[0] == 100);
	assert(node.internal.pointers[0] == MOCK(50) && node.internal.pointers[1] == MOCK(150));

	assert(middle == 200);

	assert(new_right_sibling.internal.key_count == 1);
	assert(new_right_sibling.internal.keys[0] == 300);
	assert(new_right_sibling.internal.pointers[0] == MOCK(250) &&
			new_right_sibling.internal.pointers[1] == MOCK(350));
}

static void test_insert_pointer(void) {
	btree_node_persisted node = {
		.internal = {
			.key_count = 2,
			.keys = { 100, 300 },
			.pointers = { MOCK(50), MOCK(150), MOCK(350) }
		},
	};
	insert_pointer(&node, 200, MOCK(250));
	assert(node.internal.key_count == 3);
	assert(node.internal.keys[0] = 100 && node.internal.keys[1] == 200 &&
			node.internal.keys[2] == 300);
	assert(node.internal.pointers[0] == MOCK(50) &&
			node.internal.pointers[1] == MOCK(150) &&
			node.internal.pointers[2] == MOCK(250) &&
			node.internal.pointers[3] == MOCK(350));
}

static void test_inserting(void) {
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
	btree_destroy(&tree);
}

static void test_deletion(void) {
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

void test_btree(void) {
	test_internal_splitting();
	test_insert_pointer();
	test_inserting();
	test_deletion();
}
