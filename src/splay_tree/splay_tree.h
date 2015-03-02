#ifndef SPLAY_TREE_H
#define SPLAY_TREE_H

#include <inttypes.h>
#include <stdbool.h>

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
		splay_tree_key key, bool *found, splay_tree_value* value);
int8_t splay_tree_insert(splay_tree* this,
		splay_tree_key key,
		splay_tree_value value);
// TODO: scan left, right
int8_t splay_tree_delete(splay_tree* this, splay_tree_key key);
void splay_tree_init(splay_tree** this);
void splay_tree_destroy(splay_tree** this);
// splay_up actually splays up the last node found on the path here.
void splay_up(splay_tree* tree, splay_tree_key key);

void splay_tree_next_key(splay_tree* this, splay_tree_key key,
		splay_tree_key* next_key, bool* found);
void splay_tree_previous_key(splay_tree* this, splay_tree_key key,
		splay_tree_key* previous_key, bool* found);

// TODO: delete, get previous, get next

#endif
