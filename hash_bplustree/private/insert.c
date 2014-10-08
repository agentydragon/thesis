#include "data.h"
#include "insert.h"

#include "../../log/log.h"

#include <assert.h>

typedef struct hashbplustree_node node;

static int8_t node_key_index(node* node, uint64_t key) {
	for (int i = 0; i < node->keys_count; i++) {
		if (node->keys[i] > key) return i;
	}
	return node->keys_count;
}

static void insert_into_leaf(node* node, uint64_t key, uint64_t value) {
	assert(node->keys_count < LEAF_CAPACITY);

	int8_t index = node_key_index(node, key);
	for (int8_t i = node->keys_count; i > index; i--) {
		node->keys[i] = node->keys[i - 1];
		node->values[i] = node->values[i - 1];
	}
	node->keys[index] = key;
	node->values[index] = value;
	node->keys_count++;
}

// TODO: remove duplicity?
static void insert_into_internal(node* target, uint64_t key, node* pointer) {
	assert(target->keys_count < INTERNAL_CAPACITY);

	int8_t index = node_key_index(target, key);
	for (int8_t i = target->keys_count; i > index; i--) {
		target->keys[i] = target->keys[i - 1];
		target->pointers[i] = target->pointers[i - 1];
	}
	target->keys[index] = key;
	target->pointers[index] = pointer;
	target->keys_count++;
}

void hashbplustree_split_leaf_and_add(node* leaf,
		uint64_t new_key, uint64_t new_value, node* split) {
	assert(leaf->keys_count == LEAF_CAPACITY);

	int8_t index = node_key_index(leaf, new_key);

	// Both leaves take 1/2.
	int8_t pairs_total = leaf->keys_count + 1;
	int8_t to_left = pairs_total / 2, to_right = pairs_total - to_left;

	leaf->keys_count = to_left;
	split->keys_count = to_right;
	for (int8_t i = pairs_total - 1; i >= 0; i--) {
		uint64_t key, value;
		if (i < index) {
			key = leaf->keys[i];
			value = leaf->values[i];
		} else if (i == index) {
			key = new_key;
			value = new_value;
		} else {
			key = leaf->keys[i - 1];
			value = leaf->values[i - 1];
		}

		if (i >= to_left) {
			split->keys[i - to_left] = key;
			split->values[i - to_left] = value;
		} else {
			leaf->keys[i] = key;
			leaf->values[i] = value;
		}
	}
}

// TODO: remove duplicity. generic malloc? C templates?
void hashbplustree_split_internal_and_add(node* target,
		uint64_t new_key, node* new_pointer, node* split,
		uint64_t* split_key) {
	(void) target; (void) new_key; (void) new_pointer; (void) split;
	(void) split_key;
	log_fatal("not implemented");
}

static void insert_recursion(node* target, uint64_t key, uint64_t value,
		bool* need_to_add_node, uint64_t* key_to_add,
		node** node_to_add) {
	if (!target->is_leaf) {
		int8_t index = node_key_index(target, key);
		bool i_need_to_add_node;
		uint64_t key_i_need_to_add;
		node* node_i_need_to_add;

		insert_recursion(target->pointers[index], key, value,
				&i_need_to_add_node,
				&key_i_need_to_add,
				&node_i_need_to_add);

		if (i_need_to_add_node) {
			if (target->keys_count < INTERNAL_CAPACITY) {
				insert_into_internal(target, key_i_need_to_add,
						node_i_need_to_add);
				*need_to_add_node = false;
			} else {
				// Oops, we need to split.
				node* new_internal = malloc(sizeof(node));
				new_internal->is_leaf = false;

				uint64_t new_internal_key;

				hashbplustree_split_internal_and_add(
					target, key_i_need_to_add,
					node_i_need_to_add, new_internal,
					&new_internal_key);

				log_fatal("not implemented");
			}
		}
	} else {
		// OK, we got to the leaf. Now we will try to insert this pair
		// into the leaf.
		if (target->keys_count < LEAF_CAPACITY) {
			insert_into_leaf(target, key, value);
			*need_to_add_node = false;
		} else {
			// Oops, we need to split.
			node* new_leaf = malloc(sizeof(node));
			new_leaf->is_leaf = true;

			hashbplustree_split_leaf_and_add(target, key, value,
					new_leaf);

			// Our caller will take care of inserting this node
			// one level above.
			*need_to_add_node = true;
			*node_to_add = new_leaf;

			*key_to_add = new_leaf->keys[0];
		}
	}
}

int8_t hashbplustree_insert(void* _this, uint64_t key, uint64_t value) {
	struct hashbplustree* this = _this;

	bool need_new_root;
	uint64_t new_root_key;
	node* new_branch;

	insert_recursion(this->root, key, value,
			&need_new_root, &new_root_key, &new_branch);
	if (need_new_root) {
		log_fatal("not implemented: split root");
	}
	return 0;
}
