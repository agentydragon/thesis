#include "find.h"
#include "data.h"

#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

int8_t hashbplustree_find(void* _this, uint64_t key, uint64_t* value, bool* found) {
	const struct hashbplustree* const this = _this;
	const struct hashbplustree_node* node = this->root;
	int8_t i;

	while (!node->is_leaf) {
		// TODO: maybe optimize later
		// TODO: infinite loop guard?
		assert(node->keys_count > 0);
		for (i = 0; i < node->keys_count; i++) {
			if (node->keys[i] > key) break;
		}
		node = node->pointers[i];
	}

	// TODO: maybe optimize later
	for (i = 0; i < node->keys_count; i++) {
		if (node->keys[i] == key) {
			*found = true;
			if (value) *value = node->values[i];
			return 0;
		}
	}
	*found = false;
	return 0;
}
