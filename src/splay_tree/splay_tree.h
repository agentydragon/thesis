#ifndef SPLAY_TREE_H
#define SPLAY_TREE_H

#include <inttypes.h>

typedef uint64_t splay_tree_key;
typedef uint64_t splay_tree_value;

typedef struct splay_tree_node splay_tree_node;

struct splay_tree_node {
	splay_tree_key key;
	splay_tree_value value;
	struct splay_tree_node *left, *right;
};

struct splay_tree {
	splay_tree_node* root;
};

typedef struct splay_tree splay_tree;

void splay_tree_find(splay_tree* this,
		splay_tree_key key,
		splay_tree_value* value);
void splay_tree_insert(splay_tree* this,
		splay_tree_key key,
		splay_tree_value value);
// TODO: scan left, right
void splay_tree_init(splay_tree** this);
void splay_tree_destroy(splay_tree** this);
void splay_up(splay_tree* tree, splay_tree_key key);

#endif
