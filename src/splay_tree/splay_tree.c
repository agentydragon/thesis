#include "splay_tree.h"
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "../log/log.h"

#define COUNT_OF(x) ((sizeof(x)) / sizeof(*x))
// TODO: remove log_fatals

#define GLOBAL_STACK_SIZE 1024
splay_tree_node* global_stack[GLOBAL_STACK_SIZE];

static void zig_zig_left(splay_tree_node* x, splay_tree_node* parent, splay_tree_node* grandparent) {
	splay_tree_node *a, *b, *c, *d;
	a = x->left;
	b = x->right;
	c = parent->right;
	d = grandparent->right;

	x->left = a;
	x->right = parent;
	parent->left = b;
	parent->right = grandparent;
	grandparent->left = c;
	grandparent->right = d;
}

static void zig_zig_right(splay_tree_node* x, splay_tree_node* parent, splay_tree_node* grandparent) {
	splay_tree_node *a, *b, *c, *d;
	a = grandparent->left;
	b = parent->left;
	c = x->left;
	d = x->right;

	x->left = parent;
	x->right = d;
	parent->left = grandparent;
	parent->right = c;
	grandparent->left = a;
	grandparent->right = b;
}

static void zig_zag_left_right(splay_tree_node* x, splay_tree_node* parent, splay_tree_node* grandparent) {
	splay_tree_node *a, *b, *c, *d;
	a = parent->left;
	b = x->left;
	c = x->right;
	d = grandparent->right;

	x->left = parent;
	x->right = grandparent;
	parent->left = a;
	parent->right = b;
	grandparent->left = c;
	grandparent->right = d;
}

static void zig_zag_right_left(splay_tree_node* x, splay_tree_node* parent, splay_tree_node* grandparent) {
	splay_tree_node *a, *b, *c, *d;
	a = grandparent->left;
	b = x->left;
	c = x->right;
	d = parent->right;

	x->left = grandparent;
	x->right = parent;
	grandparent->left = a;
	grandparent->right = b;
	parent->left = c;
	parent->right = d;
}

static void splay_up_internal(splay_tree* this, uint64_t key, splay_tree_node** stack, uint64_t stack_size) {
	splay_tree_node* current = this->root;

	uint64_t depth = 0;

	do {
		assert(depth < stack_size);
		stack[depth++] = current;

		if (current->key == key) {
			break;
		} else if (current->key < key) {
			if (current->right) {
				current = current->right;
			} else {
				log_fatal("failed to find");
				return;
			}
		} else /* current->key > key */ {
			if (current->left) {
				current = current->left;
			} else {
				log_fatal("failed to find");
				return;
			}
		}
	} while (true);

	while (depth > 1) {
		if (depth == 2) {
			splay_tree_node *parent = stack[0], *x = stack[1];
			if (parent->left == x) {
				splay_tree_node *b = x->right;
				x->right = parent; parent->left = b;
			} else if (parent->right == x) {
				splay_tree_node *b = x->left;
				x->left = parent; parent->right = b;
			} else {
				log_fatal("child assertion failed");
			}
			stack[0] = x;
			depth--;
		} else {
			assert(depth > 2);
			splay_tree_node *grandparent = stack[depth - 3],
					*parent = stack[depth - 2],
					*x = stack[depth - 1];

			if (grandparent->left == parent) {
				if (parent->left == x) {
					zig_zig_left(x, parent, grandparent);
				} else if (parent->right == x) {
					zig_zag_left_right(x, parent, grandparent);
				} else {
					log_fatal("child assertion failed");
				}
			} else if (grandparent->right == parent) {
				if (parent->left == x) {
					zig_zag_right_left(x, parent, grandparent);
				} else if (parent->right == x) {
					zig_zig_right(x, parent, grandparent);
				} else {
					log_fatal("child assertion failed");
				}
			} else {
				log_fatal("child assertion failed");
			}

			if (depth > 3) {
				if (stack[depth - 4]->left == stack[depth - 3]) {
					stack[depth - 4]->left = x;
				} else if (stack[depth - 4]->right == stack[depth - 3]) {
					stack[depth - 4]->right = x;
				} else {
					log_fatal("child assertion failed");
				}
			}

			stack[depth - 3] = x;
			depth -= 2;
		}
	}

	this->root = stack[0];
}

 void splay_up(splay_tree* this, uint64_t key) {
	splay_up_internal(this, key, global_stack, GLOBAL_STACK_SIZE);
}

void tree_init(struct splay_tree** _tree) {
	struct splay_tree* tree = malloc(sizeof(struct splay_tree));
	assert(tree);
	tree->root = NULL;
	*_tree = tree;
}

static void tree_destroy_recursive(struct splay_tree_node** _node) {
	assert(_node);
	if (*_node) {
		tree_destroy_recursive(&((*_node)->left));
		tree_destroy_recursive(&((*_node)->right));
		free(*_node);
		*_node = NULL;
	}
}

void tree_destroy(struct splay_tree** _tree) {
	assert(_tree);
	if (*_tree) {
		tree_destroy_recursive(&(*_tree)->root);
		free(*_tree);
		*_tree = NULL;
	}
}

static void tree_insert_recursive(uint64_t key, struct splay_tree_node* new_node,
		struct splay_tree_node* parent) {
	if (parent->key > new_node->key) {
		if (parent->left == NULL) {
			parent->left = new_node;
		} else {
			tree_insert_recursive(key, new_node, parent->left);
		}
	} else if (parent->key < new_node->key) {
		if (parent->right == NULL) {
			parent->right = new_node;
		} else {
			tree_insert_recursive(key, new_node, parent->right);
		}
	} else {
		log_fatal("element %" PRIu64 " already exists", key);
	}
}

static void tree_insert_internal(struct splay_tree* tree, uint64_t key,
		struct splay_tree_node* new_node) {
	if (tree->root == NULL) {
		tree->root = new_node;
	} else {
		tree_insert_recursive(key, new_node, tree->root);

		// This will do the splay step.
		splay_tree_find(tree, key, NULL);
	}
}

void splay_tree_insert(struct splay_tree* tree, uint64_t key, uint64_t value) {
	struct splay_tree_node* new_node = malloc(sizeof(struct splay_tree_node));
	assert(new_node);
	*new_node = (struct splay_tree_node) { .key = key, .value = value, .left = NULL, .right = NULL };
	tree_insert_internal(tree, key, new_node);
}

static void tree_find_recursive(struct splay_tree_node* current_node, uint64_t key, uint64_t *value) {
	if (current_node->key == key) {
		*value = current_node->value;
		return;
	} else if (current_node->key > key) {
		assert(current_node->left);
		tree_find_recursive(current_node->left, key, value);
	} else {
		assert(current_node->right);
		tree_find_recursive(current_node->right, key, value);
	}
}

void splay_tree_find(splay_tree* this, uint64_t key, uint64_t *value) {
	// TODO(prvak)
	if (this->root == NULL) {
		log_fatal("!!! fail");
	}

	tree_find_recursive(this->root, key, value);

	splay_up(this, key);
}
