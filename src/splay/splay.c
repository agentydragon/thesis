#include "splay/splay.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "log/log.h"

// TODO: remove log_fatals

#define node splay_node

static node* _splay(node* current, uint64_t key) {
	node root_placeholder = {
		.left = NULL,
		.right = NULL
	};
	node *left = &root_placeholder, *right = &root_placeholder;
	while (true) {
		if (key < current->key) {
			if (current->left == NULL) {
				break;
			}
			if (key < current->left->key) {
				node* tmp = current->left;
				current->left = tmp->right;
				tmp->right = current;
				current = tmp;
				if (current->left == NULL) {
					break;
				}
			}
			right->left = current;
			right = current;
			current = current->left;
		} else if (key > current->key) {
			if (current->right == NULL) {
				break;
			}
			if (key > current->right->key) {
				node* tmp = current->right;
				current->right = tmp->left;
				tmp->left = current;
				current = tmp;
				if (current->right == NULL) {
					break;
				}
			}
			left->right = current;
			left = current;
			current = current->right;
		} else {
			break;
		}
	}
	left->right = current->left;
	right->left = current->right;
	current->left = root_placeholder.right;
	current->right = root_placeholder.left;
	return current;
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

static void destroy_tree(splay_tree* this) {
	// We can't destroy nodes by DFS, because the stack could grow up
	// to O(N), which is too much. Let's delete from the maximum downward.
	// Since UINT64_MAX == DICT_RESERVED_KEY, we can find the maximum node
	// by splaying up UINT64_MAX.
	while (this->root != NULL) {
		this->root = _splay(this->root, UINT64_MAX);
		assert(!this->root->right);
		node* left = this->root->left;
		free(this->root);
		this->root = left;
	}
}

void splay_destroy(splay_tree** _tree) {
	assert(_tree);
	if (*_tree) {
		destroy_tree(*_tree);
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


void splay_find(splay_tree* this, uint64_t key, uint64_t *value, bool *found) {
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
