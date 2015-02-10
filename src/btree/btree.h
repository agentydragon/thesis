#ifndef BTREE_H
#define BTREE_H

#include <stdint.h>
#include <stdbool.h>

// Enhancement ideas:
//   - Pointers are aligned, we can drop the ends.
//   - If keys share most significant bytes, we can compress them.
//   - Custom allocator
//	=> "free block count tree":
//		each node contains lower bound on # of free blocks below

#define LEAF_MAX_KEYS 4
#define LEAF_MIN_KEYS 2  // todo??
#define INTERNAL_MAX_KEYS 3
#define INTERNAL_MIN_KEYS 1

// TODO: leaf can hold 4 keys
typedef struct btree_node {
	bool leaf;
	uint8_t key_count;
	uint64_t keys[4];

	// TODO: compress if leaf
	union {
		// FIXME: implicit expectation sizeof(uint64_t) >= sizeof(void*)
		uint64_t values[4];
		struct btree_node* pointers[4];
	};
} btree_node;

typedef struct {
	btree_node* root;
} btree;

void split_node(btree_node* node, btree_node* new_right_sibling,
		uint64_t *middle_key);
void insert_pointer(btree_node* node, uint64_t key, btree_node* pointer);
void insert_key_value_pair(btree_node* node, uint64_t key, uint64_t value);

void btree_dump(btree*);
void btree_init(btree*);
void btree_insert(btree*, uint64_t key, uint64_t value);
void btree_delete(btree*, uint64_t key);
void btree_find(btree*, uint64_t key, bool *found, uint64_t *value);
void btree_destroy(btree*);

#endif
