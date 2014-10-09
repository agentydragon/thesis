#ifndef HASHBPLUSTREE_PRIVATE_HELPERS
#define HASHBPLUSTREE_PRIVATE_HELPERS

#include <stdint.h>

typedef struct hashbplustree tree;
typedef struct hashbplustree_node node;

#define node_key_index hashbplustree_node_key_index
#define leaf_key_index hashbplustree_leaf_key_index

int8_t node_key_index(const node* node, uint64_t key);
int8_t leaf_key_index(const node* node, uint64_t key);

#endif
