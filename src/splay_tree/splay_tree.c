#include "splay_tree/splay_tree.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "log/log.h"

#define COUNT_OF(x) ((sizeof(x)) / sizeof(*x))
// TODO: remove log_fatals

#define node splay_tree_node

// TODO: rewrite to stop using single global stack
#define GLOBAL_STACK_SIZE 1024
node* global_stack[GLOBAL_STACK_SIZE];

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

static void splay_up_internal(splay_tree* this, uint64_t key, node** stack, uint64_t stack_size) {
	node* current = this->root;

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
				// log_fatal("failed to find");
				// return;
				break;
			}
		} else /* current->key > key */ {
			if (current->left) {
				current = current->left;
			} else {
				// log_fatal("failed to find");
				// return;
				break;
			}
		}
	} while (true);

	while (depth > 1) {
		if (depth == 2) {
			node *parent = stack[0], *x = stack[1];
			if (parent->left == x) {
				node *b = x->right;
				x->right = parent; parent->left = b;
			} else if (parent->right == x) {
				node *b = x->left;
				x->left = parent; parent->right = b;
			} else {
				log_fatal("child assertion failed");
			}
			stack[0] = x;
			depth--;
		} else {
			assert(depth > 2);
			node *grandparent = stack[depth - 3],
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

void splay_tree_init(struct splay_tree** _tree) {
	struct splay_tree* tree = malloc(sizeof(struct splay_tree));
	assert(tree);
	tree->root = NULL;
	*_tree = tree;
}

static void tree_destroy_recursive(node** _node) {
	assert(_node);
	if (*_node) {
		tree_destroy_recursive(&((*_node)->left));
		tree_destroy_recursive(&((*_node)->right));
		free(*_node);
		*_node = NULL;
	}
}

void splay_tree_destroy(struct splay_tree** _tree) {
	assert(_tree);
	if (*_tree) {
		tree_destroy_recursive(&(*_tree)->root);
		free(*_tree);
		*_tree = NULL;
	}
}

int8_t splay_tree_insert(struct splay_tree* tree, uint64_t key, uint64_t value) {
	node** target;
	if (tree->root == NULL) {
		target = &tree->root;
	} else {
		node* parent = tree->root;
		while (true) {
			if (parent->key > key) {
				if (parent->left == NULL) {
					target = &parent->left;
					break;
				} else {
					parent = parent->left;
				}
			} else if (parent->key < key) {
				if (parent->right == NULL) {
					target = &parent->right;
					break;
				} else {
					parent = parent->right;
				}
			} else {
				// Element already exists.
				return 1;
			}
		}
	}

	node* new_node = malloc(sizeof(node));
	assert(new_node);
	*new_node = (node) {
		.key = key, .value = value, .left = NULL, .right = NULL };
	*target = new_node;

	// This will do the splay step.
	// TODO(prvak): make this better
	splay_tree_find(tree, key, NULL, NULL);
	return 0;
}

void splay_tree_find(splay_tree* this, uint64_t key, bool *found,
		uint64_t *value) {
	if (this->root == NULL) {
		*found = false;
		return;
	}

	node* current_node = this->root;
	while (current_node->key != key) {
		if (current_node->key > key) {
			if (current_node->left) {
				current_node = current_node->left;
			} else {
				break;
			}
		} else {
			if (current_node->right) {
				current_node = current_node->right;
			} else {
				break;
			}
		}
	}

	if (found != NULL) {
		*found = (current_node->key == key);
	}
	if (current_node->key == key && value != NULL) {
		*value = current_node->value;
	}

	splay_up(this, key);
}

static uint8_t count_children(node* the_node) {
	return (the_node->left != NULL) + (the_node->right != NULL);
}

static void get_leftmost_in_right_subtree(node* current_node,
		node** _leftmost_in_right, node** _leftmost_in_right_parent) {
	node* lir_parent = current_node;
	node* leftmost_in_right = current_node->right;
	while (leftmost_in_right != NULL && leftmost_in_right->left != NULL) {
		lir_parent = leftmost_in_right;
		leftmost_in_right = leftmost_in_right->left;
	}

	*_leftmost_in_right = leftmost_in_right;
	if (_leftmost_in_right_parent) {
		*_leftmost_in_right_parent = lir_parent;
	}
}

static void get_rightmost_in_left_subtree(node* current_node,
		node** _rightmost_in_left) {
	node* rightmost_in_left = current_node->left;
	while (rightmost_in_left != NULL && rightmost_in_left->right != NULL) {
		rightmost_in_left = rightmost_in_left->right;
	}
	*_rightmost_in_left = rightmost_in_left;
}

int8_t splay_tree_delete(splay_tree* this, uint64_t key) {
	if (this->root == NULL) {
		return 1;
	}

	node* parent = NULL;
	node* current_node = this->root;
	while (current_node->key != key) {
		if (current_node->key > key) {
			if (current_node->left) {
				parent = current_node;
				current_node = current_node->left;
			} else {
				break;
			}
		} else {
			if (current_node->right) {
				parent = current_node;
				current_node = current_node->right;
			} else {
				break;
			}
		}
	}

	if (current_node->key != key) {
		return 1;
	}

	switch (count_children(current_node)) {
	case 0: {
		free(current_node);

		if (!parent) {
			// Deleting the root.
			this->root = NULL;
		} else {
			// Fix my parent and splay him up.
			if (parent->left == current_node) {
				parent->left = NULL;
			} else {
				parent->right = NULL;
			}
			splay_up(this, parent->key);
		}
		break;
	}
	case 1: {
		// I have only one child. Swap my child with me in my parent
		// and delete me.
		node* child;
		if (current_node->left) {
			child = current_node->left;
		} else {
			assert(child = current_node->right);
		}
		if (parent) {
			if (parent->left == current_node) {
				parent->left = child;
			} else {
				assert(parent->right == current_node);
				parent->right = child;
			}
			splay_up(this, parent->key);
		} else {
			this->root = child;
		}
		free(current_node);
		break;
	}
	case 2: {
		// Ooops...
		node* lir_parent;
		node* leftmost_in_right;
		get_leftmost_in_right_subtree(current_node, &leftmost_in_right,
				&lir_parent);

		if (lir_parent->left == leftmost_in_right) {
			lir_parent->left = NULL;
		} else {
			assert(lir_parent->right == leftmost_in_right);
			lir_parent->right = NULL;
		}
		current_node->key = leftmost_in_right->key;
		current_node->value = leftmost_in_right->value;
		free(leftmost_in_right);
		// TODO: maybe splay the parent of the actually deleted node
		// instead? TODO: check out the proofs
		if (parent) {
			splay_up(this, parent->key);
		}
		break;
	}
	default: {
		log_fatal("splay tree node has >2 children?");
	}
	}
	return 0;
}

static void navigate(splay_tree* this, splay_tree_key key, node** found_node) {
	node* current = this->root;
	do {
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
	*found_node = current;
}

void splay_tree_next_key(splay_tree* this, splay_tree_key key,
		splay_tree_key* next_key, bool* found) {
	node* current = this->root;
	if (current == NULL) {
		*found = false;
		return;
	}
	// TODO: navigate while splaying up
	navigate(this, key, &current);
	splay_up(this, current->key);

	if (this->root->key > key) {
		*next_key = this->root->key;
		*found = true;
	} else {
		node* leftmost_in_right;
		get_leftmost_in_right_subtree(this->root, &leftmost_in_right,
				NULL);
		if (leftmost_in_right) {
			*next_key = leftmost_in_right->key;
			*found = true;
		} else {
			*found = false;
		}
	}
}

void splay_tree_previous_key(splay_tree* this, splay_tree_key key,
		splay_tree_key* previous_key, bool *found) {
	node* current = this->root;
	if (current == NULL) {
		*found = false;
		return;
	}
	// TODO: navigate while splaying up
	navigate(this, key, &current);
	splay_up(this, current->key);

	if (this->root->key < key) {
		*previous_key = this->root->key;
		*found = true;
	} else {
		node* rightmost_in_left;
		get_rightmost_in_left_subtree(this->root, &rightmost_in_left);
		if (rightmost_in_left) {
			*previous_key = rightmost_in_left->key;
			*found = true;
		} else {
			*found = false;
		}
	}
}
