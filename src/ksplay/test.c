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

static void test_nonfull_compose() {
	ksplay_pair pairs[] = { PAIR(10), PAIR(20) };
	node* children[] = { MOCK('a'), MOCK('b'), MOCK('c') };
	node* root = ksplay_compose(pairs, children, COUNT_OF(pairs));

	assert_pairs(root, PAIR(10), PAIR(20));
	assert_children(root, MOCK('a'), MOCK('b'), MOCK('c'));
	free(root);
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

void test_ksplay() {
	test_compose();
}
