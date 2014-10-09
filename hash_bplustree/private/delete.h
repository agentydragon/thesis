#ifndef HASH_BPLUSTREE_PRIVATE_DELETE_H
#define HASH_BPLUSTREE_PRIVATE_DELETE_H

#include <stdint.h>
#include <stdbool.h>

struct hashbplustree_node;

int8_t hashbplustree_delete(void* this, uint64_t key);
void bplustree_merge_internal_nodes(struct hashbplustree_node* a,
		struct hashbplustree_node* b,
		uint64_t middle_key, bool prefer_left);

#endif
