#include "data.h"
#include "insert.h"

#include "../../log/log.h"

static int8_t node_key_index(struct hashbplustree_node* node, uint64_t key) {
	for (int i = 0; i < node->keys_count; i++) {
		if (node->keys[i] > key) return i;
	}
	return node->keys_count;
}

static void insert_into_leaf(struct hashbplustree_node* node,
		uint64_t key, uint64_t value) {
	int8_t index = node_key_index(node, key);
	for (int8_t i = node->keys_count; i > index; i--) {
		node->keys[i] = node->keys[i - 1];
		node->values[i] = node->values[i - 1];
	}
	node->keys[index] = key;
	node->values[index] = value;
	node->keys_count++;
	// TODO: check that it's not overfull
}

int8_t hashbplustree_insert(void* _this, uint64_t key, uint64_t value) {
	struct hashbplustree* this = _this;

	// Find the leaf.
	struct hashbplustree_node* node = this->root;
	while (!node->is_leaf) {
		int8_t index = node_key_index(node, key);
		struct hashbplustree_node* next = (struct hashbplustree_node*) node->values[index];
		node = next;
	}

	// OK, we got the leaf. Now we will try to insert this pair
	// into the leaf.
	if (node->keys_count == LEAF_CAPACITY) {
		// TODO TODO TODO
		log_error("insert not fully implemented");
		return 1;
	} else {
		insert_into_leaf(node, key, value);
		return 0;
	}
}
