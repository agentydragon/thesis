#include "delete.h"
#include "helpers.h"
#include "data.h"
#include "../../log/log.h"

#include <assert.h>
#include <inttypes.h>

static void delete_from_leaf(node* leaf, int8_t index) {
	for (int8_t i = index + 1; i < leaf->keys_count; i++) {
		leaf->keys[i - 1] = leaf->keys[i];
		leaf->values[i - 1] = leaf->values[i];
	}
	leaf->keys_count--;
}

static int8_t delete_recursion(node* target, uint64_t key, bool is_root) {
	if (!target->is_leaf) {
		if (delete_recursion(
				target->pointers[node_key_index(target, key)],
				key, false)) {
			// Key not found below.
			return 1;
		}

		// All is good.
		return 0;
	} else {
		if (target->keys_count >= LEAF_MINIMUM + 1 ||
				is_root) {
			// Delete from this leaf.
			int8_t index = leaf_key_index(target, key);
			if (index == -1) {
				// Key not found in leaf.
				return 1;
			} else {
				delete_from_leaf(target, index);
				return 0;
			}
		} else {
			// We need to merge :(
			log_fatal("not implemented");
			return 1;
		}
	}
}

int8_t hashbplustree_delete(void* _this, uint64_t key) {
	struct hashbplustree* this = _this;

	// (void) _this; (void) key;
	// log_error("not implemented");
	// return 1;

	if (delete_recursion(this->root, key, true)) {
		return 1;
	}
	return 0;
}
