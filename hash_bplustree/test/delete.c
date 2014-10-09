#include "delete.h"
#include "helpers.h"
#include "../private/data.h"
#include "../private/delete.h"

#include <assert.h>
#include <inttypes.h>

#include "../../log/log.h"

static void test_deleting_from_nonempty_root_leaf() {
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
}

void test_hash_bplustree_delete() {
	test_deleting_from_nonempty_root_leaf();
	// TODO
}
