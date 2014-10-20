#include "helpers.h"
#include "data.h"

#include <assert.h>

int8_t node_key_index(const node* target, uint64_t key) {
	// TODO: maybe optimize later
	for (int i = 0; i < target->keys_count; i++) {
		if (target->keys[i] > key) return i;
	}
	return target->keys_count;
}

int8_t leaf_key_index(const node* leaf, uint64_t key) {
	for (int8_t i = 0; i < leaf->keys_count; i++) {
		if (leaf->keys[i] == key) {
			return i;
		}
	}
	return -1;
}
