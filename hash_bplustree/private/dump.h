#ifndef HASH_BPLUSTREE_PRIVATE_DUMP_H
#define HASH_BPLUSTREE_PRIVATE_DUMP_H

#include <stdint.h>

struct hashbplustree_node;

void hashbplustree_dump(void* this);
void hashbplustree_dump_node(struct hashbplustree_node* target);

#endif
