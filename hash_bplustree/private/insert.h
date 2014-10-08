#ifndef HASH_BPLUSTREE_PRIVATE_INSERT_H
#define HASH_BPLUSTREE_PRIVATE_INSERT_H

#include <stdint.h>

struct hashbplustree_node;

void hashbplustree_split_leaf_and_add(struct hashbplustree_node* leaf,
		uint64_t new_key, uint64_t new_value,
		struct hashbplustree_node* split);

void hashbplustree_split_internal_and_add(struct hashbplustree_node* node,
		uint64_t new_key, struct hashbplustree_node* new_pointer,
		struct hashbplustree_node* split, uint64_t* split_key);
int8_t hashbplustree_insert(void* this, uint64_t key, uint64_t value);

#endif
