#include "btree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../log/log.h"

void btree_init(btree* this) {
	this->root = malloc(sizeof(btree_node));
	this->root->leaf = true;
	this->root->key_count = 0;
}

static void destroy_recursive(btree_node* node) {
	if (!node->leaf) {
		for (uint8_t i = 0; i < node->key_count + 1; i++) {
			destroy_recursive(node->pointers[i]);
		}
	}
	free(node);
}

void btree_destroy(btree* tree) {
	destroy_recursive(tree->root);
	tree->root = NULL;
}

static void dump_recursive(btree_node* node, int depth) {
	char buffer[256];
	uint64_t shift = 0;
	for (uint8_t i = 0; i < depth; i++) {
		shift += sprintf(buffer + shift, "  ");
	}
	if (node->leaf) {
		shift += sprintf(buffer + shift, "%p: ", node);
		for (uint8_t i = 0; i < node->key_count; ++i) {
			shift += sprintf(buffer + shift,
					" %" PRIu64 "=%" PRIu64,
					node->keys[i], node->values[i]);
		}
	} else {
		shift += sprintf(buffer + shift, "%p: ", node);
		for (uint8_t i = 0; i < node->key_count; ++i) {
			shift += sprintf(buffer + shift, " %p <%" PRIu64 ">",
					node->pointers[i], node->keys[i]);
		}
		shift += sprintf(buffer + shift, " %p",
				node->pointers[node->key_count]);
	}
	printf("%s\n", buffer);
	if (!node->leaf) {
		for (uint8_t i = 0; i < node->key_count + 1; ++i) {
			dump_recursive(node->pointers[i], depth + 1);
		}
	}
}

void btree_dump(btree* this) {
	dump_recursive(this->root, 0);
}

void split_node(btree_node* node, btree_node* new_right_sibling,
		uint64_t *middle_key) {
	// [N2] a X b Y c Z d Q e ==> [N1] a X b <- (Y) -> [N2] c Z d Q e
	if (node->leaf) {
		const uint8_t to_left = node->key_count / 2;
		const uint8_t to_right = node->key_count - to_left;

		node->key_count = to_left;
		new_right_sibling->key_count = to_right;
		new_right_sibling->leaf = true;

		for (uint8_t i = 0; i < to_right; i++) {
			new_right_sibling->keys[i] = node->keys[i + to_left];
			new_right_sibling->values[i] =
				node->values[i + to_left];
		}

		*middle_key = new_right_sibling->keys[0];
	} else {
		const uint8_t to_left = node->key_count / 2;
		const uint8_t to_right = node->key_count - to_left - 1;

		assert(to_left + 1 + to_right == node->key_count);

		node->key_count = to_left;
		new_right_sibling->key_count = to_right;
		new_right_sibling->leaf = false;

		*middle_key = node->keys[to_left];
		for (uint8_t i = to_left + 1; i < to_left + 1 + to_right; i++) {
			new_right_sibling->keys[i - to_left - 1] =
				node->keys[i];
		}
		for (uint8_t i = 0; i < to_right + 1; i++) {
			new_right_sibling->pointers[i] =
				node->pointers[to_left + 1 + i];
		}
	}
}

void insert_pointer(btree_node* node, uint64_t key, btree_node* pointer) {
	assert(!node->leaf);
	assert(node->key_count < INTERNAL_MAX_KEYS);
	uint8_t insert_at = 0xFF;
	for (uint8_t i = 0; i < node->key_count; i++) {
		if (node->keys[i] > key) {
			insert_at = i;
			break;
		}
	}
	if (insert_at == 0xFF) {
		insert_at = node->key_count;
	}
	log_info("inserting at %" PRIu8, insert_at);
	for (uint8_t i = node->key_count; i > insert_at; --i) {
		log_info("key %" PRIu8 " <- %" PRIu8 " (%" PRIu64 ")",
				i - 1, i, node->keys[i - 1]);
		node->keys[i] = node->keys[i - 1];
	}
	for (uint8_t i = node->key_count + 1; i > insert_at + 1; --i) {
		node->pointers[i] = node->pointers[i - 1];
	}
	node->keys[insert_at] = key;
	node->pointers[insert_at + 1] = pointer;
	++node->key_count;
}

void insert_key_value_pair(btree_node* leaf, uint64_t key, uint64_t value) {
	assert(leaf->leaf);
	assert(leaf->key_count < LEAF_MAX_KEYS);
	uint8_t insert_at = 0xFF;
	for (uint8_t i = 0; i < leaf->key_count; i++) {
		if (leaf->keys[i] > key) {
			insert_at = i;
			break;
		}
	}
	if (insert_at == 0xFF) {
		insert_at = leaf->key_count;
	}
	for (uint8_t i = leaf->key_count; i > insert_at; --i) {
		leaf->keys[i] = leaf->keys[i - 1];
		leaf->values[i] = leaf->values[i - 1];
	}
	leaf->keys[insert_at] = key;
	leaf->values[insert_at] = value;
	++leaf->key_count;
}

void set_new_root(btree* this, uint64_t middle_key, btree_node* left,
		btree_node* right) {
	btree_node* new_root = malloc(sizeof(btree_node));
	new_root->leaf = false;
	new_root->key_count = 1;
	new_root->keys[0] = middle_key;
	new_root->pointers[0] = left;
	new_root->pointers[1] = right;

	this->root = new_root;
}

void btree_insert(btree* this, uint64_t key, uint64_t value) {
	btree_node* parent = NULL;
	btree_node* node = this->root;

	do {
		if ((!node->leaf && node->key_count == INTERNAL_MAX_KEYS) ||
				(node->leaf && node->key_count == LEAF_MAX_KEYS)) {
			log_info("splitting %p", node);
			// We need to split the node now.
			btree_node* new_right_sibling =
					malloc(sizeof(btree_node));
			uint64_t middle_key;
			split_node(node, new_right_sibling, &middle_key);
			log_info("middle key: %" PRIu64, middle_key);
			if (parent == NULL) {
				set_new_root(this, middle_key, node,
						new_right_sibling);
				log_info("setting new root %p", this->root);
				log_info("I now look like this:");
				dump_recursive(this->root, 0);
			} else {
				insert_pointer(parent, middle_key,
						new_right_sibling);
			}
			if (key >= middle_key) {
				log_info("going to right sibling (%p)",
						new_right_sibling);
				node = new_right_sibling;
			}
		}
		if (node->leaf) {
			break;
		}
		log_info("now at: parent=%p node=%p", parent, node);
		parent = node;
		bool found = false;
		for (uint8_t i = 0; i < node->key_count; i++) {
			if (node->keys[i] > key) {
				node = node->pointers[i];
				found = true;
				break;
			}
		}
		if (!found) {
			node = node->pointers[node->key_count];
		}
		log_info("went to: parent=%p node=%p", parent, node);
	} while (true);
	insert_key_value_pair(node, key, value);
}

void btree_delete(btree* this, uint64_t key) {
	abort();
	//TODO
}

void btree_find(btree* this, uint64_t key, bool *found, uint64_t *value) {
	btree_node* node = this->root;

	while (!node->leaf) {
		bool found = false;
		for (uint8_t i = 0; i < node->key_count; i++) {
			if (node->keys[i] > key) {
				found = true;
				node = node->pointers[i];
				break;
			}
		}
		if (!found) {
			node = node->pointers[node->key_count];
		}
	}

	for (uint8_t i = 0; i < node->key_count; i++) {
		if (node->keys[i] == key) {
			*found = true;
			*value = node->values[i];
			return;
		}
	}
	*found = false;
}
