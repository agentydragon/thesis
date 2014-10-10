#include "delete.h"
#include "helpers.h"
#include "data.h"
#include "../../log/log.h"

#include <assert.h>
#include <inttypes.h>
#include <string.h>

// TODO: eagerly merge small nodes whenever traversing tree
// TODO: maybe even reshuffle?

static void delete_from_leaf(node* leaf, int8_t index) {
	for (int8_t i = index + 1; i < leaf->keys_count; i++) {
		leaf->keys[i - 1] = leaf->keys[i];
		leaf->values[i - 1] = leaf->values[i];
	}
	leaf->keys_count--;
}

static void delete_from_internal(node* target, int8_t pointer_index) {
	for (int8_t i = pointer_index; i < target->keys_count; i++) {
		target->pointers[i] = target->pointers[i + 1];
	}
	for (int8_t i = pointer_index + 1; i < target->keys_count; i++) {
		target->keys[i - 1] = target->keys[i];
	}
	target->keys_count--;
}

static void redistribute_leaf_pairs(node* a, node* b, bool* should_delete) {
	assert(a->is_leaf && b->is_leaf);

	int8_t total = a->keys_count + b->keys_count;
	int8_t to_left, to_right;

	assert(total >= LEAF_MINIMUM);

	if (total >= LEAF_MINIMUM * 2) {
		// We will just do a redistribution.
		to_left = total / 2;
		to_right = total - to_left;
		*should_delete = false;
	} else {
		assert(total <= LEAF_CAPACITY);
		// Can't feed both nodes.
		// Give all to the preferred one and sacrifice the other one
		// afterwards.
		to_left = total;
		to_right = 0;
		*should_delete = true;
	}

	// TODO: optimize?
	uint64_t keys[LEAF_CAPACITY * 2];
	uint64_t values[LEAF_CAPACITY * 2];

	for (int8_t i = 0; i < total; i++) {
		if (i < a->keys_count) {
			keys[i] = a->keys[i];
			values[i] = a->values[i];
		} else {
			keys[i] = b->keys[i - a->keys_count];
			values[i] = b->values[i - a->keys_count];
		}
	}

	a->keys_count = to_left;
	b->keys_count = to_right;
	for (int8_t i = 0; i < total; i++) {
		if (i < to_left) {
			a->keys[i] = keys[i];
			a->values[i] = values[i];
		} else {
			b->keys[i - to_left] = keys[i];
			b->values[i - to_left] = values[i];
		}
	}
}

static const uint64_t INVALID = 0xDEADBEEFDEADBEEFull;

static int8_t delete_recursion(node* target, uint64_t key, bool is_root,
		node* left_sibling, node* right_sibling,
		uint64_t target_key, uint64_t right_sibling_key,
		bool* should_bubble_up_delete, uint64_t* to_bubble_delete) {
	if (!target->is_leaf) {
		int8_t index = node_key_index(target, key);

		node* next_left_sibling = index > 0 ? target->pointers[index - 1] : NULL;
		uint64_t next_target_key = index > 0 ? target->keys[index - 1] : INVALID;

		node* next_right_sibling = index < target->keys_count ?
				target->pointers[index + 1] : NULL;
		uint64_t next_right_sibling_key = index < target->keys_count ?
				target->keys[index] : INVALID;

		bool should_delete;
		uint64_t i_should_delete = INVALID;

		if (delete_recursion(
				target->pointers[index],
				key, false,
				next_left_sibling, next_right_sibling,
				next_target_key, next_right_sibling_key,
				&should_delete, &i_should_delete)) {
			// Key not found below.
			*should_bubble_up_delete = false;
			return 1;
		}

		// All is good.
		*should_bubble_up_delete = false;

		if (should_delete) {
			int8_t index_to_delete = node_key_index(target, i_should_delete);
			free(target->pointers[index_to_delete]);
			delete_from_internal(target, index_to_delete);

			// NOTE: this is a special case of node merging usable only
			// for 2-3-4 trees.
			if (target->keys_count < INTERNAL_MINIMUM) {
				assert(target->keys_count == 0);
				node* to_delete = target->pointers[0];
				memcpy(target, target->pointers[0], sizeof(node));
				free(to_delete);
			}
		}

		return 0;
	} else {
		// Delete from this leaf.
		int8_t index = leaf_key_index(target, key);
		if (index == -1) {
			// Key not found in leaf.
			*should_bubble_up_delete = false;
			return 1;
		}
		delete_from_leaf(target, index);

		if (target->keys_count < LEAF_MINIMUM && !is_root) {
			// Need to merge.
			if (left_sibling != NULL && (right_sibling == NULL ||
					 left_sibling->keys_count > right_sibling->keys_count)) {
				redistribute_leaf_pairs(left_sibling, target,
						should_bubble_up_delete);
				*to_bubble_delete = target_key;
			} else if (right_sibling != NULL) {
				redistribute_leaf_pairs(target, right_sibling,
						should_bubble_up_delete);
				*to_bubble_delete = right_sibling_key;
			} else {
				log_fatal("no siblings passed, but not in root");
			}
		}

		return 0;
	}
}

int8_t hashbplustree_delete(void* _this, uint64_t key) {
	struct hashbplustree* this = _this;

	bool should_delete_root = false;
	if (delete_recursion(this->root, key, true, NULL, NULL,
				-1, -1,
				&should_delete_root, NULL)) {
		return 1;
	}
	if (should_delete_root) {
		node* new_root = this->root->pointers[0];
		free(this->root);
		this->root = new_root;
	}

	return 0;
}
