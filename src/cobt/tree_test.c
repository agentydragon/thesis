#include "cobt/tree_test.h"

#include <assert.h>

#include "cobt/tree.h"

void test_cobt_tree(void) {
	bool occupied[] = { true, true, true, true, true };
	uint64_t array[] = { 10, 99999, 20, 99999, 30 };
	const uint64_t size = sizeof(array) / sizeof(*array);

	cobt_tree tree;
	cobt_tree_init(&tree, array, occupied, size);
	assert(cobt_tree_find_le(&tree, 0) == 0);
	assert(cobt_tree_find_le(&tree, 10) == 0);
	assert(cobt_tree_find_le(&tree, 15) == 0);
	assert(cobt_tree_find_le(&tree, 20) == 2);
	assert(cobt_tree_find_le(&tree, 25) == 2);
	assert(cobt_tree_find_le(&tree, 30) == 4);
	assert(cobt_tree_find_le(&tree, 35) == 4);

	// 10, 15, 25, 99999, 30
	array[1] = 15; array[2] = 25;
	cobt_tree_refresh(&tree, (cobt_tree_range) {
		.begin = 1,
		.end = 3
	});
	assert(cobt_tree_find_le(&tree, 0) == 0);
	assert(cobt_tree_find_le(&tree, 10) == 0);
	assert(cobt_tree_find_le(&tree, 12) == 0);
	assert(cobt_tree_find_le(&tree, 15) == 1);
	assert(cobt_tree_find_le(&tree, 20) == 1);
	assert(cobt_tree_find_le(&tree, 25) == 2);
	assert(cobt_tree_find_le(&tree, 27) == 2);
	assert(cobt_tree_find_le(&tree, 30) == 4);
	assert(cobt_tree_find_le(&tree, 35) == 4);

	cobt_tree_destroy(&tree);
}
