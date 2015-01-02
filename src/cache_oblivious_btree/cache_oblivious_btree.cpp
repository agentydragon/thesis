#include "cache_oblivious_btree.h"
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "../log/log.h"
#include "../math/math.h"
#include "../veb_layout/veb_layout.h"

namespace {

bool parameters_equal(struct parameters x, struct parameters y) {
	return x.block_size == y.block_size && x.capacity == y.capacity;
}

bool range_find_first_gt(
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

bool range_find_last_lt(
		struct ordered_file file, struct ordered_file_range range,
		uint64_t key, uint64_t *index) {
	bool found_before = false;
	uint64_t index_before = COB_INFINITY;
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

bool range_find(
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

uint64_t range_get_minimum(struct ordered_file file,
		struct ordered_file_range range) {
	// TODO: optimize. should just be the first item!
	uint64_t minimum = COB_INFINITY;
	for (uint64_t i = 0; i < range.size; i++) {
		if (file.occupied[range.begin + i] &&
				file.keys[range.begin + i] < minimum) {
			const uint64_t key = file.keys[range.begin + i];
			if (minimum > key) {
				minimum = key;
			}
			assert(minimum < COB_INFINITY);  // sanity check
		}
	}
	return minimum;
}

struct ordered_file_range insert_sorted_order(struct ordered_file* file,
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

}

uint64_t cob::leaf_range_count() const {
	const struct parameters parameters = file.parameters;
	assert(parameters.capacity % parameters.block_size == 0);
	return parameters.capacity / parameters.block_size;
}

uint64_t cob::get_veb_height() const {
	return exact_log2(leaf_range_count()) + 1;
}

void cob::fix_internal_node(uint64_t veb_node) {
	const veb_pointer left = veb_get_left(veb_node, get_veb_height());
	const veb_pointer right = veb_get_right(veb_node, get_veb_height());
	assert(left.present && right.present);

	log_info("below %" PRIu64 ": [%" PRIu64 "]=%" PRIu64 ", "
			"[%" PRIu64 "]=%" PRIu64,
			veb_node, left.node, veb_minima[left.node],
			right.node, veb_minima[right.node]);

	const uint64_t a = veb_minima[left.node],
		      b = veb_minima[right.node];
	veb_minima[veb_node] = (a < b) ? a : b;
}

void cob::fix_stack(uint64_t* stack, uint64_t stack_size) {
	log_info("fixing stack of size %" PRIu64, stack_size);
	for (uint64_t i = 0; i < stack_size; i++) {
		const uint64_t node = stack[stack_size - 1 - i];
		log_info("fixing node %" PRIu64, node);
		fix_internal_node(node);
	}
}

void cob::recalculate_minima(uint64_t veb_node) {
	log_info("recalculating everything under %" PRIu64, veb_node);
	if (veb_is_leaf(veb_node, get_veb_height())) {
		const uint64_t leaf_number =
				veb_get_leaf_index_of_leaf(veb_node,
						get_veb_height());
		log_info("leaf_number=%" PRIu64, leaf_number);
		const uint64_t leaf_offset = leaf_number *
				file.parameters.block_size;

		veb_minima[veb_node] = range_get_minimum(
				file, (struct ordered_file_range) {
					.begin = leaf_offset,
					.size = file.parameters.block_size,
				});
		log_info("%" PRIu64 " is a leaf. new minimum is %" PRIu64,
				veb_node, veb_minima[veb_node]);
	} else {
		const veb_pointer left =
			veb_get_left(veb_node, get_veb_height());
		const veb_pointer right =
			veb_get_right(veb_node, get_veb_height());
		assert(left.present && right.present);

		recalculate_minima(left.node);
		recalculate_minima(right.node);

		fix_internal_node(veb_node);
	}
}

void cob::veb_walk(uint64_t key, uint64_t* stack, uint64_t* _stack_size,
		uint64_t* _leaf_index) const {
	log_info("veb_walking to key %" PRIu64, key);
	uint64_t stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t pointer = 0;  // 0 == van Emde Boas root node
	uint64_t leaf_index = 0;
	do {
		stack[stack_size++] = pointer;

		if (veb_is_leaf(pointer, get_veb_height())) {
			log_info("-> %" PRIu64 " is the leaf we want", pointer);
			// This is the leaf.
			break;
		} else {
			const veb_pointer left =
				veb_get_left(pointer, get_veb_height());
			const veb_pointer right =
				veb_get_right(pointer, get_veb_height());
			log_info("-> %" PRIu64 ": right min = %" PRIu64,
					pointer, veb_minima[right.node]);

			if (key >= veb_minima[right.node]) {
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

void cob::entirely_reset_veb() {
	veb_minima = (uint64_t*) realloc(veb_minima,
			file.parameters.capacity * sizeof(uint64_t));
	assert(veb_minima);
	recalculate_minima(0);
}

void cob::insert(uint64_t key) {
	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(key, node_stack, &node_stack_size, &leaf_index);

	const struct parameters prior_parameters = file.parameters;

	// Insert into ordered file.
	const struct ordered_file_range reorg_range = insert_sorted_order(
			&file, get_leaf_range(file, leaf_index), key);

	if (parameters_equal(file.parameters, prior_parameters)) {
		const uint64_t levels_up = exact_log2(
				reorg_range.size / file.parameters.block_size);

		// Rebuild reorganized subtree.
		recalculate_minima(node_stack[node_stack_size - 1 - levels_up]);
		fix_stack(node_stack, node_stack_size - levels_up - 1);
	} else {
		entirely_reset_veb();
	}
}

void cob::remove(uint64_t key) {
	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(key, node_stack, &node_stack_size, &leaf_index);

	const struct parameters prior_parameters = file.parameters;

	// Delete from ordered file.
	struct ordered_file_range leaf_sr = get_leaf_range(file, leaf_index);
	uint64_t index;
	if (!range_find(file, leaf_sr, key, &index)) {
		log_fatal("deleting nonexistant key %" PRIu64 " "
				"(leaf index %" PRIu64 ")", key, leaf_index);
	}
	const struct ordered_file_range reorg_range = ordered_file_delete(
			&file, index);
	if (parameters_equal(file.parameters, prior_parameters)) {
		const uint64_t levels_up = exact_log2(
				reorg_range.size / file.parameters.block_size);
		log_info("levels_up=%" PRIu64);

		// Rebuild reorganized subtree.
		recalculate_minima(node_stack[node_stack_size - 1 - levels_up]);
		fix_stack(node_stack, node_stack_size - levels_up - 1);
	} else {
		entirely_reset_veb();
	}
}

void cob::has_key(uint64_t key, bool *found) const {
	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(key, node_stack, &node_stack_size, &leaf_index);
	*found = range_find(file,
			get_leaf_range(file, leaf_index), key, NULL);
}

void cob::next_key(uint64_t key,
		bool *next_key_exists, uint64_t* next_key) const {
	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(key, node_stack, &node_stack_size, &leaf_index);
	struct ordered_file_range leaf = get_leaf_range(file, leaf_index);

	// TODO: plain single FOR?
	while (leaf_index < leaf_range_count()) {
		uint64_t index;
		if (range_find_first_gt(file, leaf, key, &index)) {
			*next_key_exists = true;
			*next_key = file.keys[index];
			return;
		}
		leaf.begin += leaf.size;
		leaf_index++;
	}

	*next_key_exists = false;
}

void cob::previous_key(uint64_t key,
		bool *previous_key_exists, uint64_t* previous_key) const {
	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(key, node_stack, &node_stack_size, &leaf_index);
	struct ordered_file_range leaf = get_leaf_range(file, leaf_index);

	// TODO: plain single FOR?
	do {
		uint64_t index;
		if (range_find_last_lt(file, leaf, key, &index)) {
			*previous_key_exists = true;
			*previous_key = file.keys[index];
			return;
		}
		leaf.begin -= leaf.size;
	} while (leaf_index-- > 0);

	*previous_key_exists = false;
}
