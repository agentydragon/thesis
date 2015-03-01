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

typedef struct btree_node_persisted {
	union {
		struct {
			uint8_t key_count;
			uint64_t keys[INTERNAL_MAX_KEYS];
			struct btree_node_persisted* pointers[
				INTERNAL_MAX_KEYS + 1];
		} internal;

		struct {
			uint64_t keys[LEAF_MAX_KEYS];
			uint64_t values[LEAF_MAX_KEYS];
		} leaf;
	};
} btree_node_persisted;

typedef struct {
	uint8_t levels_above_leaves;  // levels_above_leaves 0 == just root
	btree_node_persisted* root;
} btree;

void split_leaf(btree_node_persisted* node,
		btree_node_persisted* new_right_sibling, uint64_t *middle_key);
void split_internal(btree_node_persisted* node,
		btree_node_persisted* new_right_sibling, uint64_t *middle_key);
void insert_pointer(btree_node_persisted* node, uint64_t key,
		btree_node_persisted* pointer);
int8_t insert_key_value_pair(btree_node_persisted* node,
		uint64_t key, uint64_t value);

void btree_dump(btree*);
void btree_init(btree*);
int8_t btree_insert(btree*, uint64_t key, uint64_t value);
int8_t btree_delete(btree*, uint64_t key);
void btree_find(btree*, uint64_t key, bool *found, uint64_t *value);
void btree_destroy(btree*);

#endif