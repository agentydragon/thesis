#include "splay_tree/test.h"

#include <assert.h>

#include "log/log.h"
#include "splay_tree/splay_tree.h"

#define MOCK(x) ((struct splay_tree_node*) x)

#define assert_node(expect,found_ptr) do { \
	const splay_tree_node expected = expect; \
	const splay_tree_node got = *(found_ptr); \
	if (got.key != expected.key) { \
		log_error("%s:%d: error: got.key (%" PRIu64 ") != " \
				"expected.key (%" PRIu64 ")\n", \
				__FILE__, __LINE__, got.key, expected.key); \
		assert(got.key == expected.key); \
	} \
	assert(got.left == expected.left); \
	assert(got.right == expected.right); \
} while (0)

static void test_alternating_3() {
	splay_tree_node c = { .key = 5,  .left = MOCK(2), .right = MOCK(3) };
	splay_tree_node b = { .key = 1,  .left = MOCK(1), .right = &c };
	splay_tree_node a = { .key = 10, .left = &b, .right = MOCK(4) };

	splay_tree this = { .root = &a };
	splay_up(&this, 5);

	assert(this.root == &c);
	assert_node(((struct splay_tree_node) { .key = 5, .left = &b, .right = &a }), &c);
	assert_node(((struct splay_tree_node) { .key = 1, .left = MOCK(1), .right = MOCK(2) }), &b);
	assert_node(((struct splay_tree_node) { .key = 10, .left = MOCK(3), .right = MOCK(4) }), &a);
}

static void test_right_path_4() {
	splay_tree_node d = { .key = 400, .left = MOCK(4), .right = MOCK(5) };
	splay_tree_node c = { .key = 300, .left = MOCK(3), .right = &d };
	splay_tree_node b = { .key = 200, .left = MOCK(2), .right = &c };
	splay_tree_node a = { .key = 100, .left = MOCK(1), .right = &b };

	splay_tree this = { .root = &a };
	splay_up(&this, 400);

	assert(this.root == &d);
	assert_node(((struct splay_tree_node) { .key = 400, .left = &a, .right = MOCK(5) }), &d);
	assert_node(((struct splay_tree_node) { .key = 100, .left = MOCK(1), .right = &c }), &a);
	assert_node(((struct splay_tree_node) { .key = 300, .left = &b, .right = MOCK(4) }), &c);
	assert_node(((struct splay_tree_node) { .key = 200, .left = MOCK(2), .right = MOCK(3) }), &b);
}

static void test_left_path_5() {
	splay_tree_node e = { .key = 600, .left = MOCK(1), .right = MOCK(2) };
	splay_tree_node d = { .key = 700, .left = &e, .right = MOCK(3) };
	splay_tree_node c = { .key = 800, .left = &d, .right = MOCK(4) };
	splay_tree_node b = { .key = 900, .left = &c, .right = MOCK(5) };
	splay_tree_node a = { .key = 1000, .left = &b, .right = MOCK(6) };

	splay_tree this = { .root = &a };
	splay_up(&this, 600);

	assert(this.root == &e);
	assert_node(((struct splay_tree_node) { .key = 600, .left = MOCK(1), .right = &b }), &e);
	assert_node(((struct splay_tree_node) { .key = 900, .left = &d, .right = &a }), &b);
	assert_node(((struct splay_tree_node) { .key = 700, .left = MOCK(2), .right = &c }), &d);
	assert_node(((struct splay_tree_node) { .key = 1000, .left = MOCK(5), .right = MOCK(6) }), &a);
	assert_node(((struct splay_tree_node) { .key = 800, .left = MOCK(3), .right = MOCK(4) }), &c);
}

void test_splay_tree() {
	test_alternating_3();
	test_right_path_4();
	test_left_path_5();
}
