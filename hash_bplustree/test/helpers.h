#ifndef HASHBPLUSTREE_TEST_HELPERS
#define HASHBPLUSTREE_TEST_HELPERS

#include <stdint.h>

typedef struct hashbplustree_node node;
typedef struct hashbplustree tree;

#define assert_key bplustree_test_assert_key
#define assert_value bplustree_test_assert_value
#define assert_pointer bplustree_test_assert_pointer
#define assert_n_keys bplustree_test_assert_n_keys

void assert_key(const node* const target, uint64_t index, uint64_t expected);
void assert_value(const node* const target, uint64_t index, uint64_t expected);
void assert_pointer(const node* const target, uint64_t index,
		const void* const expected);
void assert_n_keys(const node* const target, uint64_t expected);

#define assert_leaf(_node,...) do { \
	const node* const target = _node; \
	struct { uint64_t key, value; } pairs[] = { __VA_ARGS__ }; \
	uint64_t N = sizeof(pairs) / sizeof(*pairs); \
	assert_n_keys(target, N); \
	for (uint64_t i = 0; i < N; i++) { \
		assert_key(target, i, pairs[i].key); \
		assert_value(target, i, pairs[i].value); \
	} \
} while (0)

#endif
