#include "cache_oblivious_btree.h"
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../math/math.h"
#include "../veb_layout/veb_layout.h"

#define INFINITY 0xFFFFFFFFDEADBEEF

static bool range_find_first_gt(
		struct ordered_file file, struct ordered_file_range range,
		uint64_t key, uint64_t *index) {
	for (uint64_t i = 0; i < range.size; i++) {
		const uint64_t j = range.begin + i;
		if (file.occupied[j] && file.keys[j] > key) {
			*index = j;
			return true;
		}
	}
	return false;
}

static bool range_find_last_lt(
		struct ordered_file file, struct ordered_file_range range,
		uint64_t key, uint64_t *index) {
	bool found_before = false;
	uint64_t index_before = INFINITY;
	for (uint64_t i = 0; i < range.size; i++) {
		if (file.occupied[range.begin + i]) {
			if (found_before && file.keys[range.begin + i] >= key) {
				*index = index_before;
				return true;
			}

			if (file.keys[range.begin + i] < key) {
				found_before = true;
				index_before = range.begin + i;
			}
		}
	}
	if (found_before) {
		*index = index_before;
		return true;
	} else {
		return false;
	}
}

static bool range_find(
		struct ordered_file file, struct ordered_file_range range,
		uint64_t key, uint64_t *index) {
	for (uint64_t i = 0; i < range.size; i++) {
		if (file.occupied[range.begin + i] &&
				file.keys[range.begin + i] == key) {
			if (index != NULL) {
				*index = range.begin + i;
			}
			return true;
		}
	}
	return false;
}

static uint64_t range_get_minimum(struct ordered_file file,
		struct ordered_file_range range) {
	// TODO: optimize. should just be the first item!
	uint64_t minimum = INFINITY;
	for (uint64_t i = 0; i < range.size; i++) {
		if (file.occupied[range.begin + i] &&
				file.keys[range.begin + i] < minimum) {
			const uint64_t key = file.keys[range.begin + i];
			if (minimum > key) {
				minimum = key;
			}
			assert(minimum < INFINITY);  // sanity check
		}
	}
	return minimum;
}

static struct ordered_file_range insert_sorted_order(struct ordered_file* file,
		struct ordered_file_range range, uint64_t inserted_item) {
	bool found_before = false;
	uint64_t index_before;
	for (uint64_t i = 0; i < range.size; i++) {
		if (file->occupied[range.begin + i]) {
			if (found_before &&
					file->keys[range.begin + i] > inserted_item) {
				// Insert after index_before.
				break;
			}

			if (file->keys[range.begin + i] <= inserted_item) {
				found_before = true;
				index_before = i;
			}
		}
	}

	if (found_before) {
		const uint64_t insert_after_index = range.begin + index_before;
		log_info("inserting %" PRIu64 " after %" PRIu64,
				inserted_item, insert_after_index);
		return ordered_file_insert_after(file,
				inserted_item, insert_after_index);
	} else {
		assert(range.begin == 0);
		return ordered_file_insert_first(file, inserted_item);
	}
}

static uint64_t leaf_range_count(const struct cob* this) {
	const struct parameters parameters = this->file.parameters;
	assert(parameters.capacity % parameters.block_size == 0);
	return parameters.capacity / parameters.block_size;
}

uint64_t get_veb_height(const struct cob* this) {
	return exact_log2(leaf_range_count(this)) + 1;
}

void cob_fix_internal_node(struct cob* this, uint64_t veb_node) {
	const veb_pointer left = veb_get_left(veb_node, get_veb_height(this));
	const veb_pointer right = veb_get_right(veb_node, get_veb_height(this));
	assert(left.present && right.present);

	log_info("below %" PRIu64 ": [%" PRIu64 "]=%" PRIu64 ", "
			"[%" PRIu64 "]=%" PRIu64,
			veb_node, left.node, this->veb_minima[left.node],
			right.node, this->veb_minima[right.node]);

	const uint64_t a = this->veb_minima[left.node],
		      b = this->veb_minima[right.node];
	this->veb_minima[veb_node] = (a < b) ? a : b;
}

void cob_fix_stack(struct cob* this, uint64_t* stack, uint64_t stack_size) {
	log_info("fixing stack of size %" PRIu64, stack_size);
	for (uint64_t i = 0; i < stack_size; i++) {
		const uint64_t node = stack[stack_size - 1 - i];
		log_info("fixing node %" PRIu64, node);
		cob_fix_internal_node(this, node);
	}
}

void cob_recalculate_minima(struct cob* this, uint64_t veb_node) {
	log_info("recalculating everything under %" PRIu64, veb_node);
	if (veb_is_leaf(veb_node, get_veb_height(this))) {
		const uint64_t leaf_number =
				veb_get_leaf_index_of_leaf(veb_node,
						get_veb_height(this));
		log_info("leaf_number=%" PRIu64, leaf_number);
		const uint64_t leaf_offset = leaf_number *
				this->file.parameters.block_size;

		this->veb_minima[veb_node] = range_get_minimum(
				this->file, (struct ordered_file_range) {
					.begin = leaf_offset,
					.size = this->file.parameters.block_size,
				});
		log_info("%" PRIu64 " is a leaf. new minimum is %" PRIu64,
				veb_node, this->veb_minima[veb_node]);
	} else {
		const veb_pointer left =
			veb_get_left(veb_node, get_veb_height(this));
		const veb_pointer right =
			veb_get_right(veb_node, get_veb_height(this));
		assert(left.present && right.present);

		cob_recalculate_minima(this, left.node);
		cob_recalculate_minima(this, right.node);

		cob_fix_internal_node(this, veb_node);
	}
}

static void veb_walk(const struct cob* this, uint64_t key,
		uint64_t* stack, uint64_t* _stack_size,
		uint64_t* _leaf_index) {
	log_info("veb_walking to key %" PRIu64, key);
	uint64_t stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t pointer = 0;  // 0 == van Emde Boas root node
	uint64_t leaf_index = 0;
	do {
		stack[stack_size++] = pointer;

		if (veb_is_leaf(pointer, get_veb_height(this))) {
			log_info("-> %" PRIu64 " is the leaf we want", pointer);
			// This is the leaf.
			break;
		} else {
			const veb_pointer left =
				veb_get_left(pointer, get_veb_height(this));
			const veb_pointer right =
				veb_get_right(pointer, get_veb_height(this));
			log_info("-> %" PRIu64 ": right min = %" PRIu64,
					pointer, this->veb_minima[right.node]);

			if (key >= this->veb_minima[right.node]) {
				// We want to go right.
				pointer = right.node;
				leaf_index = (leaf_index << 1) + 1;
			} else {
				// We want to go left.
				pointer = left.node;
				leaf_index = leaf_index << 1;
			}
		}
	} while (true);

	*_stack_size = stack_size;
	*_leaf_index = leaf_index;
}

void cob_insert(struct cob* this, uint64_t key) {
	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);

	// Insert into ordered file.
	const struct ordered_file_range reorg_range = insert_sorted_order(
			&this->file, get_leaf_range(this->file, leaf_index), key);
	const uint64_t levels_up = exact_log2(
			reorg_range.size / this->file.parameters.block_size);

	// Rebuild reorganized subtree.
	cob_recalculate_minima(this, node_stack[node_stack_size - 1 - levels_up]);
	cob_fix_stack(this, node_stack, node_stack_size - levels_up - 1);
}

void cob_delete(struct cob* this, uint64_t key) {
	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);

	// Insert into ordered file.
	struct ordered_file_range leaf_sr = get_leaf_range(this->file,
			leaf_index);
	uint64_t index;
	if (!range_find(this->file, leaf_sr, key, &index)) {
		log_fatal("deleting nonexistant key %" PRIu64 " "
				"(leaf index %" PRIu64 ")", key, leaf_index);
	}
	const struct ordered_file_range reorg_range = ordered_file_delete(
			&this->file, index);
	const uint64_t levels_up = exact_log2(
			reorg_range.size / this->file.parameters.block_size);
	log_info("levels_up=%" PRIu64);

	// Rebuild reorganized subtree.
	cob_recalculate_minima(this, node_stack[node_stack_size - 1 - levels_up]);
	cob_fix_stack(this, node_stack, node_stack_size - levels_up - 1);
}

// TODO: test
void cob_has_key(const struct cob* this, uint64_t key, bool *found) {
	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);
	*found = range_find(this->file,
			get_leaf_range(this->file, leaf_index), key, NULL);
}

// TODO: test
void cob_next_key(const struct cob* this, uint64_t key,
		bool *next_key_exists, uint64_t* next_key) {
	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);
	struct ordered_file_range leaf = get_leaf_range(this->file, leaf_index);

	// TODO: plain single FOR?
	while (leaf_index < leaf_range_count(this)) {
		uint64_t index;
		if (range_find_first_gt(this->file, leaf, key, &index)) {
			*next_key_exists = true;
			*next_key = this->file.keys[index];
			return;
		}
		leaf.begin += leaf.size;
		leaf_index++;
	}

	*next_key_exists = false;
}

// TODO: test
void cob_previous_key(const struct cob* this, uint64_t key,
		bool *previous_key_exists, uint64_t* previous_key) {
	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);
	struct ordered_file_range leaf = get_leaf_range(this->file, leaf_index);

	// TODO: plain single FOR?
	do {
		uint64_t index;
		if (range_find_last_lt(this->file, leaf, key, &index)) {
			*previous_key_exists = true;
			*previous_key = this->file.keys[index];
			return;
		}
		leaf.begin -= leaf.size;
	} while (leaf_index-- > 0);

	*previous_key_exists = false;
}
