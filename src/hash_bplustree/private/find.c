#include "find.h"
#include "data.h"
#include "helpers.h"

#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

int8_t hashbplustree_find(void* _this, uint64_t key, uint64_t* value, bool* found) {
	const tree* const this = _this;
	const node* target = this->root;

	while (!target->is_leaf) {
		target = target->pointers[node_key_index(target, key)];
	}

	// TODO: maybe optimize later
	int8_t index = leaf_key_index(target, key);
	if (index == -1) {
		*found = false;
	} else {
		*found = true;
		if (value) *value = target->values[index];
	}
	return 0;
}
