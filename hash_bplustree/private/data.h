#ifndef HASH_BPLUSTREE_PRIVATE_DATA_H
#define HASH_BPLUSTREE_PRIVATE_DATA_H

#include <stdint.h>
#include <stdbool.h>

// TODO: rename
#define LEAF_CAPACITY 3
#define INTERNAL_CAPACITY 3

// Enhancement ideas:
//   - Pointers are aligned, we can drop the ends.
//   - If keys share most significant bytes, we can compress them.
//   - Custom allocator
//	=> "free block count tree":
//		each node contains lower bound on # of free blocks below

struct hashbplustree_node {
	bool is_leaf;
	uint8_t keys_count;

	uint64_t keys[3];

	union {
		// FIXME: implicit expectation sizeof(uint64_t) >= sizeof(void*)
		uint64_t values[3];
		struct hashbplustree_node* pointers[4];
	};

	// sizeof == 64 B
};

struct hashbplustree {
	struct hashbplustree_node* root;
};

#endif
