#ifndef SPLAY_H
#define SPLAY_H

#include <inttypes.h>
#include <stdbool.h>
#include "util/unused.h"

typedef uint64_t splay_key;
typedef uint64_t splay_value;

typedef struct splay_node {
	splay_key key;
	splay_value value;
	struct splay_node *left, *right;
} splay_node;

typedef struct {
	splay_node* root;
} splay_tree;

bool splay_find(splay_tree* this, splay_key key, splay_value* value);
bool splay_insert(splay_tree* this, splay_key key, splay_value value);
bool splay_delete(splay_tree* this, splay_key key);
void splay_init(splay_tree** this);
void splay_destroy(splay_tree** this);

// Actually splays up the last node found on the path here.
void splay(splay_tree* tree, splay_key key);

bool splay_next_key(splay_tree* this, splay_key key, splay_key* next_key);
bool splay_previous_key(splay_tree* this, splay_key key,
		splay_key* previous_key);

#endif
