#include "splay/splay.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "log/log.h"

#define COUNT_OF(x) ((sizeof(x)) / sizeof(*x))
// TODO: remove log_fatals

#define node splay_node

// TODO: rewrite to stop using single global stack
// TODO: the stack can actually be O(N) worst-case
#define GLOBAL_STACK_SIZE 1024
node* GLOBAL_STACK_CONTENT[GLOBAL_STACK_SIZE];

typedef struct {
	node** stack;
	uint64_t depth;
	uint64_t capacity;
} splay_stack;

#define GLOBAL_STACK (splay_stack) {   \
	.stack = GLOBAL_STACK_CONTENT, \
	.depth = 0,                    \
	.capacity = GLOBAL_STACK_SIZE  \
}

static void zig_zig_left(node* x, node* parent, node* grandparent) {
	node *a, *b, *c, *d;
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

static void zig_zig_right(node* x, node* parent, node* grandparent) {
	node *a, *b, *c, *d;
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

static void zig_zag_left_right(node* x, node* parent, node* grandparent) {
	node *a, *b, *c, *d;
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

static void zig_zag_right_left(node* x, node* parent, node* grandparent) {
	node *a, *b, *c, *d;
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

static node* navigate(node* current, splay_key key,
		splay_stack* stack) {
	assert(stack);
	stack->depth = 0;
	do {
		assert(stack->depth < stack->capacity);
		stack->stack[stack->depth++] = current;

		if (current->key == key) {
			break;  // Got it.
		} else if (key < current->key) {
			if (current->left) {
				current = current->left;
			} else {
				break;
			}
		} else {
			if (current->right) {
				current = current->right;
			} else {
				break;
			}
		}
	} while (true);
	return current;
}

static node* splay_up_stack(splay_stack* stack) {
	while (stack->depth > 1) {
		if (stack->depth == 2) {
			node *parent = stack->stack[0], *x = stack->stack[1];
			if (parent->left == x) {
				node *b = x->right;
				x->right = parent; parent->left = b;
			} else if (parent->right == x) {
				node *b = x->left;
				x->left = parent; parent->right = b;
			} else {
				log_fatal("child assertion failed");
			}
			stack->stack[0] = x;
			stack->depth--;
		} else {
			assert(stack->depth > 2);
			node *grandparent = stack->stack[stack->depth - 3],
					*parent = stack->stack[stack->depth - 2],
					*x = stack->stack[stack->depth - 1];

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

			if (stack->depth > 3) {
				if (stack->stack[stack->depth - 4]->left == stack->stack[stack->depth - 3]) {
					stack->stack[stack->depth - 4]->left = x;
				} else if (stack->stack[stack->depth - 4]->right == stack->stack[stack->depth - 3]) {
					stack->stack[stack->depth - 4]->right = x;
				} else {
					log_fatal("child assertion failed");
				}
			}

			stack->stack[stack->depth - 3] = x;
			stack->depth -= 2;
		}
	}

	return stack->stack[0];
}

static node* _splay(node* root, uint64_t key) {
	splay_stack stack = GLOBAL_STACK;
	navigate(root, key, &stack);
	return splay_up_stack(&stack);
}

void splay(splay_tree* this, uint64_t key) {
	this->root = _splay(this->root, key);
}


void splay_init(splay_tree** _tree) {
	splay_tree* tree = malloc(sizeof(splay_tree));
	assert(tree);
	tree->root = NULL;
	*_tree = tree;
}

static void destroy_recursive(node** _node) {
	assert(_node);
	if (*_node) {
		destroy_recursive(&((*_node)->left));
		destroy_recursive(&((*_node)->right));
		free(*_node);
		*_node = NULL;
	}
}

void splay_destroy(splay_tree** _tree) {
	assert(_tree);
	if (*_tree) {
		destroy_recursive(&(*_tree)->root);
		free(*_tree);
		*_tree = NULL;
	}
}

static node* make_node(uint64_t key, uint64_t value) {
	node* new_node = malloc(sizeof(node));
	*new_node = (node) {
		.key = key,
		.value = value
	};
	return new_node;
}

int8_t splay_insert(splay_tree* tree, uint64_t key, uint64_t value) {
	node* new_node;
	if (tree->root == NULL) {
		new_node = make_node(key, value);
		new_node->left = NULL;
		new_node->right = NULL;
	} else {
		node* parent = _splay(tree->root, key);
		if (parent->key == key) {
			// Element already exists.
			tree->root = parent;
			return 1;
		}
		new_node = make_node(key, value);
		if (key < parent->key) {
			new_node->left = parent->left;
			new_node->right = parent;
			parent->left = NULL;
		} else if (key > parent->key) {
			new_node->right = parent->right;
			new_node->left = parent;
			parent->right = NULL;
		} else {
			log_fatal("parent->key == key handled above");
		}
	}
	tree->root = new_node;
	return 0;
}


void splay_find(splay_tree* this, uint64_t key, uint64_t *value,
		bool *found) {
	if (this->root == NULL) {
		*found = false;
		return;
	}

	node* parent = _splay(this->root, key);
	this->root = parent;
	if (found != NULL) {
		*found = (this->root->key == key);
	}
	if (this->root->key == key && value != NULL) {
		*value = this->root->value;
	}
}

static void get_leftmost_in_right_subtree(node* current,
		node** _leftmost_in_right, node** _leftmost_in_right_parent) {
	node* lir_parent = current;
	node* leftmost_in_right = current->right;
	while (leftmost_in_right != NULL && leftmost_in_right->left != NULL) {
		lir_parent = leftmost_in_right;
		leftmost_in_right = leftmost_in_right->left;
	}

	*_leftmost_in_right = leftmost_in_right;
	if (_leftmost_in_right_parent) {
		*_leftmost_in_right_parent = lir_parent;
	}
}

static void get_rightmost_in_left_subtree(node* current,
		node** _rightmost_in_left) {
	node* rightmost_in_left = current->left;
	while (rightmost_in_left != NULL && rightmost_in_left->right != NULL) {
		rightmost_in_left = rightmost_in_left->right;
	}
	*_rightmost_in_left = rightmost_in_left;
}

int8_t splay_delete(splay_tree* this, uint64_t key) {
	if (this->root == NULL) {
		return 1;
	}

	node* deleted = _splay(this->root, key);
	if (deleted->key != key) {
		return 1;
	}

	if (deleted->left == NULL) {
		this->root = deleted->right;
	} else {
		// Splay up the largest node in left subtree.
		// This node will have no right child, so we can collapse it.
		this->root = _splay(deleted->left, key);
		assert(this->root->right == NULL);
		this->root->right = deleted->right;
	}
	free(deleted);
	return 0;
}

void splay_next_key(splay_tree* this, splay_key key,
		splay_key* next_key, bool* found) {
	if (this->root == NULL) {
		*found = false;
		return;
	}

	this->root = _splay(this->root, key);
	if (this->root->key > key) {
		if (next_key) {
			*next_key = this->root->key;
		}
		*found = true;
	} else {
		node* leftmost_in_right;
		// TODO: splay
		get_leftmost_in_right_subtree(this->root, &leftmost_in_right,
				NULL);
		if (leftmost_in_right) {
			if (next_key) {
				*next_key = leftmost_in_right->key;
			}
			*found = true;
		} else {
			*found = false;
		}
	}
}

void splay_previous_key(splay_tree* this, splay_key key,
		splay_key* previous_key, bool *found) {
	if (this->root == NULL) {
		*found = false;
		return;
	}

	this->root = _splay(this->root, key);

	if (this->root->key < key) {
		if (previous_key) {
			*previous_key = this->root->key;
		}
		*found = true;
	} else {
		node* rightmost_in_left;
		// TODO: splay
		get_rightmost_in_left_subtree(this->root, &rightmost_in_left);
		if (rightmost_in_left) {
			if (previous_key) {
				*previous_key = rightmost_in_left->key;
			}
			*found = true;
		} else {
			*found = false;
		}
	}
}
