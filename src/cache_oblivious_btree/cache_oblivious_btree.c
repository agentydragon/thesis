#include "cache_oblivious_btree.h"
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "../math/math.h"
#include "../veb_layout/veb_layout.h"

#define NO_LOG_INFO
#include "../log/log.h"

static bool parameters_equal(struct parameters x, struct parameters y) {
	return x.block_size == y.block_size && x.capacity == y.capacity;
}

static bool range_find_first_gt(
		struct ordered_file file, struct ordered_file_range range,
		uint64_t key, uint64_t *index) {
	for (uint64_t i = 0; i < range.size; i++) {
		const uint64_t j = range.begin + i;
		if (file.occupied[j] && file.items[j].key > key) {
			*index = j;
			return true;
		}
	}
	return false;
}

static bool range_find_last_lt(
		struct ordered_file file, struct ordered_file_range range,
		uint64_t key, uint64_t *found_index) {
	bool found_before = false;
	uint64_t index_before = COB_INFINITY;
	for (uint64_t i = 0; i < range.size; i++) {
		const uint64_t index = range.begin + i;
		if (file.occupied[index]) {
			if (found_before && file.items[index].key >= key) {
				*found_index = index_before;
				return true;
			}

			if (file.items[index].key < key) {
				found_before = true;
				index_before = index;
			}
		}
	}
	if (found_before) {
		*found_index = index_before;
		return true;
	} else {
		return false;
	}
}

static bool range_find(
		struct ordered_file file, struct ordered_file_range range,
		uint64_t key, uint64_t *found_index) {
	for (uint64_t i = 0; i < range.size; i++) {
		const uint64_t index = range.begin + i;
		if (file.occupied[index] && file.items[index].key == key) {
			if (found_index != NULL) {
				*found_index = index;
			}
			return true;
		}
	}
	return false;
}

static uint64_t range_get_minimum(struct ordered_file file,
		struct ordered_file_range range) {
	// Since items are stored in ascending order, the first present item
	// is the minimum.
	for (uint64_t i = 0; i < range.size; i++) {
		const uint64_t index = range.begin + i;
		if (file.occupied[index]) {
			return file.items[index].key;
		}
	}
	return COB_INFINITY;
}

static struct ordered_file_range insert_sorted_order(struct ordered_file* file,
		struct ordered_file_range range, uint64_t key, uint64_t value) {
	bool found_before = false;
	uint64_t index_before;
	for (uint64_t i = 0; i < range.size; i++) {
		const uint64_t index = range.begin + i;
		if (file->occupied[index]) {
			if (found_before && file->items[index].key > key) {
				// Insert after index_before.
				break;
			}

			if (file->items[index].key <= key) {
				found_before = true;
				index_before = i;
			}
		}
	}

	if (found_before) {
		const uint64_t insert_after_index = range.begin + index_before;
		log_info("inserting %" PRIu64 "=%" PRIu64 " after %" PRIu64,
				key, insert_after_index);
		return ordered_file_insert_after(file,
				(ordered_file_item) {
					.key = key,
					.value = value
				}, insert_after_index);
	} else {
		assert(range.begin == 0);
		return ordered_file_insert_first(file,
				(ordered_file_item) {
					.key = key,
					.value = value
				});
	}
}

static uint64_t leaf_range_count(struct cob this) {
	const struct parameters parameters = this.file.parameters;
	assert(parameters.capacity % parameters.block_size == 0);
	return parameters.capacity / parameters.block_size;
}

uint64_t get_veb_height(struct cob this) {
	return exact_log2(leaf_range_count(this)) + 1;
}

static void cob_fix_internal_node(struct cob* this, uint64_t veb_node) {
	veb_pointer left, right;
	veb_get_children(veb_node, get_veb_height(*this), &left, &right);
	assert(left.present && right.present);

	log_info("below %" PRIu64 ": [%" PRIu64 "]=%" PRIu64 ", "
			"[%" PRIu64 "]=%" PRIu64,
			veb_node, left.node, this->veb_minima[left.node],
			right.node, this->veb_minima[right.node]);

	const uint64_t a = this->veb_minima[left.node],
		      b = this->veb_minima[right.node];
	this->veb_minima[veb_node] = (a < b) ? a : b;
}

static void cob_fix_stack(struct cob* this,
		uint64_t* stack, uint64_t stack_size) {
	log_info("fixing stack of size %" PRIu64, stack_size);
	for (uint64_t i = 0; i < stack_size; i++) {
		const uint64_t node = stack[stack_size - 1 - i];
		log_info("fixing node %" PRIu64, node);
		cob_fix_internal_node(this, node);
	}
}

// levels_up is passed as an optimization.
void cob_recalculate_minima(struct cob* this, uint64_t veb_node,
		uint64_t levels_up) {
	log_info("recalculating everything under %" PRIu64, veb_node);
	// Save veb_height for optimization.
	const uint64_t veb_height = get_veb_height(*this);
	// assert(veb_is_leaf(veb_node, veb_height) == (levels_up == 0));
	if (levels_up == 0) {
		const uint64_t leaf_number =
				veb_get_leaf_index_of_leaf(veb_node,
						veb_height);
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
		veb_pointer left, right;
		veb_get_children(veb_node, veb_height, &left, &right);
		assert(left.present && right.present);

		cob_recalculate_minima(this, left.node, levels_up - 1);
		cob_recalculate_minima(this, right.node, levels_up - 1);

		cob_fix_internal_node(this, veb_node);
	}
}

static void veb_walk(const struct cob* this, uint64_t key,
		uint64_t* stack, uint64_t* _stack_size,
		uint64_t* _leaf_index) {
	log_info("veb_walking to key %" PRIu64, key);
	uint64_t stack_size = 0;

	const uint64_t veb_height = get_veb_height(*this);
	// Walk down vEB layout to find where does the key belong.
	uint64_t pointer = 0;  // 0 == van Emde Boas root node
	uint64_t leaf_index = 0;
	do {
		stack[stack_size++] = pointer;

		if (stack_size == veb_height) {
			log_info("-> %" PRIu64 " is the leaf we want", pointer);
			// This is the leaf.
			break;
		} else {
			veb_pointer left, right;
			veb_get_children(pointer, veb_height, &left, &right);
			log_info("-> %" PRIu64 ": right min = %" PRIu64,
					pointer, this->veb_minima[right.node]);
			CHECK(left.present && right.present, "unexpected leaf");

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

static void entirely_reset_veb(struct cob* this) {
	this->veb_minima = realloc(this->veb_minima,
			this->file.parameters.capacity * sizeof(uint64_t));
	assert(this->veb_minima);
	cob_recalculate_minima(this, 0, get_veb_height(*this) - 1);
}

static void validate_key(uint64_t key) {
	CHECK(key != COB_INFINITY, "Trying to operate on key=COB_INFINITY");
}

void cob_insert(struct cob* this, uint64_t key, uint64_t value) {
	validate_key(key);

	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);

	const struct parameters prior_parameters = this->file.parameters;

	// Insert into ordered file.
	const struct ordered_file_range reorg_range = insert_sorted_order(
			&this->file, get_leaf_range(this->file, leaf_index),
			key, value);

	if (parameters_equal(this->file.parameters, prior_parameters)) {
		const uint64_t levels_up = exact_log2(
				reorg_range.size / this->file.parameters.block_size);

		// Rebuild reorganized subtree.
		cob_recalculate_minima(this,
				node_stack[node_stack_size - 1 - levels_up],
				levels_up);
		cob_fix_stack(this, node_stack, node_stack_size - levels_up - 1);
	} else {
		entirely_reset_veb(this);
	}
}

int8_t cob_delete(struct cob* this, uint64_t key) {
	validate_key(key);

	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);

	const struct parameters prior_parameters = this->file.parameters;

	// Delete from ordered file.
	struct ordered_file_range leaf_sr = get_leaf_range(this->file,
			leaf_index);
	uint64_t index;
	if (!range_find(this->file, leaf_sr, key, &index)) {
		// Deleting nonexistant key.
		return 1;
	}
	const struct ordered_file_range reorg_range = ordered_file_delete(
			&this->file, index);
	if (parameters_equal(this->file.parameters, prior_parameters)) {
		const uint64_t levels_up = exact_log2(
				reorg_range.size / this->file.parameters.block_size);

		// Rebuild reorganized subtree.
		cob_recalculate_minima(this,
				node_stack[node_stack_size - 1 - levels_up],
				levels_up);
		cob_fix_stack(this, node_stack, node_stack_size - levels_up - 1);
	} else {
		entirely_reset_veb(this);
	}
	return 0;
}

void cob_find(const struct cob* this, uint64_t key,
		bool *found, uint64_t *value) {
	validate_key(key);

	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);

	const struct ordered_file_range leaf_range = get_leaf_range(this->file, leaf_index);
	uint64_t index;
	if (range_find(this->file, leaf_range, key, &index)) {
		*found = true;
		if (value) {
			*value = this->file.items[index].value;
		}
	} else {
		*found = false;
	}
}

void cob_next_key(const struct cob* this, uint64_t key,
		bool *next_key_exists, uint64_t* next_key) {
	validate_key(key);

	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);
	struct ordered_file_range leaf = get_leaf_range(this->file, leaf_index);

	// TODO: plain single FOR?
	while (leaf_index < leaf_range_count(*this)) {
		uint64_t index;
		if (range_find_first_gt(this->file, leaf, key, &index)) {
			*next_key_exists = true;
			*next_key = this->file.items[index].key;
			return;
		}
		leaf.begin += leaf.size;
		leaf_index++;
	}

	*next_key_exists = false;
}

void cob_previous_key(const struct cob* this, uint64_t key,
		bool *previous_key_exists, uint64_t* previous_key) {
	validate_key(key);

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
			*previous_key = this->file.items[index].key;
			return;
		}
		leaf.begin -= leaf.size;
	} while (leaf_index-- > 0);

	*previous_key_exists = false;
}

void cob_init(struct cob* this) {
	ordered_file_init(&this->file);
	this->veb_minima = NULL;
	entirely_reset_veb(this);
}

void cob_destroy(struct cob this) {
	ordered_file_destroy(this.file);
	free(this.veb_minima);
}
