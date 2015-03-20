#include "ksplay/test.h"

#include <assert.h>
#include <stdlib.h>

#include "ksplay/ksplay.h"
#include "log/log.h"

#define node ksplay_node

#define COUNT_OF(x) (sizeof(x) / sizeof(*x))
#define PAIR(x) ((ksplay_pair) { .key = x, .value = x * 100 })
#define MOCK(x) ((node*) x)

static void assert_pair(node* x, uint8_t index, ksplay_pair expected_pair) {
	assert(x->pairs[index].key == expected_pair.key);
	assert(x->pairs[index].value == expected_pair.value);
}

#define assert_pairs(x,...) do { \
	ksplay_pair _pairs[] = { __VA_ARGS__ }; \
	assert(x->key_count == COUNT_OF(_pairs)); \
	for (uint64_t i = 0; i < COUNT_OF(_pairs); ++i) { \
		assert_pair(x, i, _pairs[i]); \
	} \
} while (0)

static void _assert_children(node* x, node** children, uint64_t child_count) {
	for (uint64_t i = 0; i < child_count; ++i) {
		assert(x->children[i] == children[i]);
	}
}

#define assert_children(x,...) do { \
	node* _children[] = { __VA_ARGS__ }; \
	_assert_children(x, _children, COUNT_OF(_children)); \
} while (0)

#define set_pairs(x,...) do { \
	node* _node = (x); \
	ksplay_pair _pairs[] = { __VA_ARGS__ }; \
	_node->key_count = COUNT_OF(_pairs); \
	for (uint8_t i = 0; i < COUNT_OF(_pairs); ++i) { \
		_node->pairs[i] = _pairs[i]; \
	} \
} while (0)

#define set_children(x,...) do { \
	node* _node = (x); \
	node* _children[] = { __VA_ARGS__ }; \
	assert(COUNT_OF(_children) == _node->key_count + 1); \
	for (uint8_t i = 0; i < COUNT_OF(_children); ++i) { \
		_node->children[i] = _children[i]; \
	} \
} while (0)

static void test_nonfull_compose() {
	ksplay_pair pairs[] = { PAIR(10), PAIR(20) };
	node* children[] = { MOCK('a'), MOCK('b'), MOCK('c') };
	node* root = ksplay_compose(pairs, children, COUNT_OF(pairs));

	assert_pairs(root, PAIR(10), PAIR(20));
	assert_children(root, MOCK('a'), MOCK('b'), MOCK('c'));
	free(root);
}

static void test_walk_to() {
	node a, b, c, root;
	set_pairs(&a, PAIR(48), PAIR(49));
	a.children[0] = a.children[1] = a.children[2] = NULL;

	set_pairs(&b, PAIR(30), PAIR(45));
	b.children[0] = b.children[1] = NULL; b.children[2] = &a;

	set_pairs(&c, PAIR(25), PAIR(50), PAIR(75));
	c.children[0] = NULL;
	c.children[1] = &b;
	c.children[2] = c.children[3] = NULL;

	set_pairs(&root, PAIR(100), PAIR(200));
	root.children[0] = &c;
	root.children[1] = root.children[2] = root.children[3] = NULL;

	ksplay tree = { .root = &root };
	ksplay_node_buffer stack = ksplay_walk_to(&tree, 47);
	assert(stack.count == 4);
	assert(stack.nodes[0] == &root && stack.nodes[1] == &c &&
			stack.nodes[2] == &b && stack.nodes[3] == &a);
	free(stack.nodes);
}

static void test_exact_compose() {
	ksplay_pair pairs[] = {
		PAIR(10), PAIR(20), PAIR(30), PAIR(40),
		PAIR(50), PAIR(60), PAIR(70), PAIR(80)
	};
	node* children[] = {
		MOCK('A'), MOCK('B'), MOCK('C'), MOCK('D'), MOCK('E'),
		MOCK('F'), MOCK('G'), MOCK('H'), MOCK('I')
	};
	node* root = ksplay_compose(pairs, children, COUNT_OF(pairs));

	assert_pairs(root, PAIR(30), PAIR(60));
	node *left = root->children[0], *middle = root->children[1],
			*right = root->children[2];

	assert_pairs(left, PAIR(10), PAIR(20));
	assert_children(left, MOCK('A'), MOCK('B'), MOCK('C'));
	assert_pairs(middle, PAIR(40), PAIR(50));
	assert_children(middle, MOCK('D'), MOCK('E'), MOCK('F'));
	assert_pairs(right, PAIR(70), PAIR(80));
	assert_children(right, MOCK('G'), MOCK('H'), MOCK('I'));

	free(left); free(middle); free(right); free(root);
}

static void test_underfull_compose() {
	ksplay_pair pairs[] = {
		PAIR(10), PAIR(20), PAIR(30), PAIR(40), PAIR(50)
	};
	node* children[] = {
		MOCK('a'), MOCK('b'), MOCK('c'), MOCK('d'), MOCK('e'), MOCK('f')
	};
	node* root = ksplay_compose(pairs, children, COUNT_OF(pairs));
	assert_pairs(root, PAIR(30));

	node* left = root->children[0], *right = root->children[1];
	assert_pairs(left, PAIR(10), PAIR(20));
	assert_children(left, MOCK('a'), MOCK('b'), MOCK('c'));
	assert_pairs(right, PAIR(40), PAIR(50));
	assert_children(right, MOCK('d'), MOCK('e'), MOCK('f'));

	free(left); free(right); free(root);
}

static void test_overfull_compose() {
	ksplay_pair pairs[] = {
		PAIR(10), PAIR(20), PAIR(30), PAIR(40), PAIR(50), PAIR(60),
		PAIR(70), PAIR(80), PAIR(90), PAIR(100), PAIR(110)
	};
	node* children[] = {
		MOCK('a'), MOCK('b'), MOCK('c'), MOCK('d'), MOCK('e'),
		MOCK('f'), MOCK('g'), MOCK('h'), MOCK('i'), MOCK('j'),
		MOCK('k'), MOCK('l')
	};
	node* root = ksplay_compose(pairs, children, COUNT_OF(pairs));
	assert_pairs(root, PAIR(30), PAIR(60), PAIR(90));

	node* a = root->children[0], *b = root->children[1],
			*c = root->children[2], *d = root->children[3];
	assert_pairs(a, PAIR(10), PAIR(20));
	assert_children(a, MOCK('a'), MOCK('b'), MOCK('c'));
	assert_pairs(b, PAIR(40), PAIR(50));
	assert_children(b, MOCK('d'), MOCK('e'), MOCK('f'));
	assert_pairs(c, PAIR(70), PAIR(80));
	assert_children(c, MOCK('g'), MOCK('h'), MOCK('i'));
	assert_pairs(d, PAIR(100), PAIR(110));
	assert_children(d, MOCK('j'), MOCK('k'), MOCK('l'));

	free(a); free(b); free(c); free(d); free(root);
}

static void test_mixed_compose() {
	// Terminal splay.
	ksplay_pair pairs[] = {
		PAIR(10), PAIR(20), PAIR(30), PAIR(40)
	};
	node* children[] = {
		MOCK('a'), MOCK('b'), MOCK('c'), MOCK('d'), MOCK('e')
	};
	node* root = ksplay_compose(pairs, children, COUNT_OF(pairs));
	assert_pairs(root, PAIR(30), PAIR(40));

	node* child = root->children[0];
	assert_pairs(child, PAIR(10), PAIR(20));
	assert_children(child, MOCK('a'), MOCK('b'), MOCK('c'));
	assert(root->children[1] == MOCK('d'));
	assert(root->children[2] == MOCK('e'));
}

static void test_compose() {
	test_nonfull_compose();
	test_exact_compose();
	test_underfull_compose();
	test_overfull_compose();
	test_mixed_compose();
}

#define assert_pairs_equal(pairs,...) do { \
	ksplay_pair _expected[] = { __VA_ARGS__ }; \
	for (uint64_t i = 0; i < COUNT_OF(_expected); ++i) { \
		assert(_expected[i].key == pairs[i].key); \
		assert(_expected[i].value == pairs[i].value); \
	} \
} while (0)

static void test_flatten() {
	node a, b, c;
	set_pairs(&a, PAIR(100));
	set_children(&a, MOCK('A'), MOCK('B'));

	set_pairs(&b, PAIR(20), PAIR(50));
	set_children(&b, MOCK('C'), MOCK('D'), &a);

	set_pairs(&c, PAIR(10), PAIR(500));
	set_children(&c, MOCK('E'), &b, MOCK('F'));

	ksplay_pair* pairs;
	node** children;
	ksplay_node_buffer stack = {
		.count = 3,
		.capacity = 3,
		.nodes = (ksplay_node*[]) { &c, &b, &a }
	};
	uint64_t key_count;
	ksplay_flatten(&stack, &pairs, &children, &key_count);

	assert(key_count == 5);
	assert_pairs_equal(pairs, PAIR(10), PAIR(20), PAIR(50), PAIR(100),
			PAIR(500));
	assert(children[0] == MOCK('E') && children[1] == MOCK('C') &&
			children[2] == MOCK('D') && children[3] == MOCK('A') &&
			children[4] == MOCK('B') && children[5] == MOCK('F'));
	free(pairs); free(children);
}

#define assert_not_found(key) do { \
	bool _found; \
	ksplay_find(&tree, key, NULL, &_found); \
	assert(!_found); \
} while (0)

#define assert_found(key,value) do { \
	bool _found; \
	uint64_t _found_value; \
	ksplay_find(&tree, key, &_found_value, &_found); \
	assert(_found && _found_value == value); \
} while (0)

static void test_insert() {
	ksplay tree;
	ksplay_init(&tree);
	assert_not_found(10);
	//ksplay_dump(&tree);

	ksplay_insert(&tree, 10, 100);
	//ksplay_dump(&tree);
	assert_found(10, 100);

	ksplay_insert(&tree, 20, 200);
	//ksplay_dump(&tree);
	assert_found(10, 100);
	assert_found(20, 200);

	ksplay_insert(&tree, 30, 300);
	//ksplay_dump(&tree);
	assert_found(10, 100);
	assert_found(20, 200);
	assert_found(30, 300);

	uint64_t keys[] = {15, 25, 5, 50, 40, 35, 45};
	for (uint64_t i = 0; i < COUNT_OF(keys); ++i) {
		ksplay_insert(&tree, keys[i], keys[i] * 10);
	}

	uint64_t found_keys[] = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50};
	for (uint64_t i = 0; i < COUNT_OF(found_keys); ++i) {
		assert_found(found_keys[i], found_keys[i] * 10);
	}

	ksplay_destroy(&tree);
}

void test_ksplay() {
	test_compose();
	test_walk_to();
	test_flatten();
	test_insert();
}
