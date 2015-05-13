#ifndef BTREE_H
#define BTREE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Enhancement ideas:
//   - Pointers are aligned, we can drop the ends.
//   - If keys share most significant bytes, we can compress them.
//   - Custom allocator
//	=> "free block count tree":
//		each node contains lower bound on # of free blocks below

// #define NODE_BYTES 64
// #define NODE_BYTES 128
#define NODE_BYTES 256  // 256 B nodes did best on our test.
// #define NODE_BYTES 512
// #define NODE_BYTES 1024
#define LEAF_MAX_KEYS (NODE_BYTES / (sizeof(uint64_t) * 2))
#define LEAF_MIN_KEYS (LEAF_MAX_KEYS / 2)
// TODO: Why does this need the typecast?
#define INTERNAL_MAX_KEYS ((int) ((NODE_BYTES - sizeof(void*)) / (sizeof(uint64_t) + sizeof(void*))))
#define INTERNAL_MIN_KEYS (INTERNAL_MAX_KEYS / 2)

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

void btree_init(btree*);
bool btree_insert(btree*, uint64_t key, uint64_t value);
bool btree_delete(btree*, uint64_t key);
bool btree_find(btree*, uint64_t key, uint64_t *value);
void btree_destroy(btree*);

bool btree_find_next(btree*, uint64_t key, uint64_t *next_key);
bool btree_find_prev(btree*, uint64_t key, uint64_t *prev_key);

typedef struct {
	uint8_t levels_above_leaves;
	btree_node_persisted* persisted;
} btree_node_traversed;

bool nt_is_leaf(btree_node_traversed node);
btree_node_traversed nt_root(btree* tree);
uint8_t get_n_leaf_keys(const btree_node_persisted* node);

void btree_dump_dot(const btree* this, FILE* output);

typedef struct {
	uint64_t internal_n_keys_histogram[100];
	uint64_t total_kvp_path_length;
	uint64_t total_kvps;
} btree_stats;

btree_stats btree_collect_stats(btree* this);

#endif
