#include "data.h"
#include "helpers.h"
#include "insert.h"
#include "dump.h"

#include "../../log/log.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

static void insert_into_leaf(node* leaf, uint64_t key, uint64_t value) {
	assert(leaf->keys_count < LEAF_CAPACITY);

	int8_t index = node_key_index(leaf, key);
	for (int8_t i = leaf->keys_count; i > index; i--) {
		leaf->keys[i] = leaf->keys[i - 1];
		leaf->values[i] = leaf->values[i - 1];
	}
	leaf->keys[index] = key;
	leaf->values[index] = value;
	leaf->keys_count++;
}

// TODO: remove duplicity?
static void insert_into_internal(node* target, uint64_t key, node* pointer) {
	assert(target->keys_count < INTERNAL_CAPACITY);

	int8_t index = node_key_index(target, key);
	for (int8_t i = target->keys_count; i > index; i--) {
		target->pointers[i + 1] = target->pointers[i];
	}
	for (int8_t i = target->keys_count; i > index; i--) {
		target->keys[i] = target->keys[i - 1];
	}
	target->keys[index] = key;
	target->pointers[index + 1] = pointer;
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

void hashbplustree_split_internal_and_add(node* target,
		uint64_t new_key, node* new_pointer, node* split,
		uint64_t* split_key) {
	assert(target->keys_count == INTERNAL_CAPACITY);

	int8_t index = node_key_index(target, new_key);

	// TODO: optimize and unduplicate

	uint64_t keys[INTERNAL_CAPACITY + 1];
	node* pointers[INTERNAL_CAPACITY + 2];

	pointers[0] = target->pointers[0];

	for (int8_t i = 0; i < INTERNAL_CAPACITY + 1; i++) {
		uint64_t key;
		node* pointer;

		if (i < index) {
			key = target->keys[i];
			pointer = target->pointers[i + 1];
		} else if (i == index) {
			key = new_key;
			pointer = new_pointer;
		} else {
			key = target->keys[i - 1];
			pointer = target->pointers[i];
		}

		keys[i] = key;
		pointers[i + 1] = pointer;
	}

	int8_t right_count = INTERNAL_CAPACITY / 2,
		left_count = INTERNAL_CAPACITY - right_count;

	target->keys_count = left_count;
	target->pointers[0] = pointers[0];
	for (int8_t i = 0; i < left_count; i++) {
		target->keys[i] = keys[i];
		target->pointers[i + 1] = pointers[i + 1];
	}

	*split_key = keys[left_count];

	split->keys_count = right_count;
	split->pointers[0] = pointers[left_count + 1];
	for (int8_t i = 0; i < right_count; i++) {
		split->keys[i] = keys[left_count + 1 + i];
		split->pointers[i + 1] = pointers[left_count + 2 + i];
	}
}

static void insert_recursion(node* target, uint64_t key, uint64_t value,
		bool* need_to_add_node, uint64_t* key_to_add,
		node** node_to_add) {
	if (!target->is_leaf) {
		int8_t index = node_key_index(target, key);
		bool i_need_to_add_node;
		uint64_t key_i_need_to_add;
		node* node_i_need_to_add;

		assert(target->pointers[index]);
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
				assert(new_internal);
				new_internal->keys_count = 0;
				new_internal->is_leaf = false;

				uint64_t new_internal_key;

				hashbplustree_split_internal_and_add(
					target, key_i_need_to_add,
					node_i_need_to_add, new_internal,
					&new_internal_key);

				*need_to_add_node = true;
				*key_to_add = new_internal_key,
				*node_to_add = new_internal;
			}
		} else {
			*need_to_add_node = false;
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
			assert(new_leaf);
			// TODO: proper error checking, custom allocator?
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
		node* new_root = malloc(sizeof(node));
		assert(new_root); // TODO: proper error handling
		*new_root = (node) {
			.is_leaf = false,
			.keys_count = 1,
			.keys = { new_root_key },
			.pointers = { this->root, new_branch }
		};
		this->root = new_root;
	}
	return 0;
}
