#include "cache_oblivious_btree.h"
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "../math/math.h"
#include "../veb_layout/veb_layout.h"

#define NO_LOG_INFO
#include "../log/log.h"

struct parameters { uint64_t block_size; uint64_t capacity; };

static struct parameters get_params(ofm file) {
	return (struct parameters) {
		.block_size = file.block_size,
		.capacity = file.capacity
	};
}

static bool parameters_equal(struct parameters x, struct parameters y) {
	return x.block_size == y.block_size && x.capacity == y.capacity;
}

static bool range_find(ofm_range range, uint64_t key, uint64_t *found_index) {
	for (uint64_t i = 0; i < range.size; i++) {
		const uint64_t index = range.begin + i;
		if (range.file->occupied[index] &&
				range.file->items[index].key == key) {
			if (found_index != NULL) {
				*found_index = index;
			}
			return true;
		}
	}
	return false;
}

static uint64_t range_get_minimum(ofm_range range) {
	// Since items are stored in ascending order, the first present item
	// is the minimum.
	for (uint64_t i = 0; i < range.size; i++) {
		const uint64_t index = range.begin + i;
		if (range.file->occupied[index]) {
			return range.file->items[index].key;
		}
	}
	return COB_INFINITY;
}

static ofm_range insert_sorted_order(ofm_range range, uint64_t key,
		uint64_t value) {
	bool found_after = false;
	uint64_t index_after;
	for (uint64_t i = range.begin; i < range.file->capacity; i++) {
		if (range.file->occupied[i] && range.file->items[i].key > key) {
			found_after = true;
			index_after = i;
			break;
		}
	}

	ofm_range touched_range;
	if (found_after) {
		//log_info("inserting %" PRIu64 "=%" PRIu64 " after %" PRIu64,
		//		key, insert_after_index);
		ofm_insert_before(range.file,
				(ofm_item) { .key = key, .value = value },
				index_after, NULL, &touched_range);
	} else {
		ofm_insert_before(range.file,
				(ofm_item) { .key = key, .value = value },
				range.file->capacity, NULL, &touched_range);
	}
	return touched_range;
}

static uint64_t leaf_range_count(struct cob this) {
	assert(this.file.capacity % this.file.block_size == 0);
	return this.file.capacity / this.file.block_size;
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
				this->file.block_size;

		this->veb_minima[veb_node] = range_get_minimum( (ofm_range) {
					.begin = leaf_offset,
					.size = this->file.block_size,
					.file = &this->file
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
			this->file.capacity * sizeof(uint64_t));
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

	const struct parameters prior_parameters = get_params(this->file);

	// Insert into ordered file.
	const ofm_range reorg_range = insert_sorted_order(
			ofm_get_leaf(&this->file,
					leaf_index * this->file.block_size),
			key, value);

	if (parameters_equal(get_params(this->file), prior_parameters)) {
		const uint64_t levels_up = exact_log2(
				reorg_range.size / this->file.block_size);

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

	const struct parameters prior_parameters = get_params(this->file);

	// Delete from ordered file.
	ofm_range leaf_sr = ofm_get_leaf(&this->file, leaf_index * this->file.block_size);
	uint64_t index;
	if (!range_find(leaf_sr, key, &index)) {
		// Deleting nonexistant key.
		return 1;
	}
	ofm_range reorg_range;
	ofm_delete(&this->file, index, NULL, &reorg_range);
	if (parameters_equal(get_params(this->file), prior_parameters)) {
		const uint64_t levels_up = exact_log2(
				reorg_range.size / this->file.block_size);

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

void cob_find(struct cob* this, uint64_t key, bool *found, uint64_t *value) {
	validate_key(key);

	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);

	const ofm_range leaf_range = ofm_get_leaf(&this->file,
			leaf_index * this->file.block_size);
	uint64_t index;
	if (range_find(leaf_range, key, &index)) {
		*found = true;
		if (value) {
			*value = this->file.items[index].value;
		}
	} else {
		*found = false;
	}
}

void cob_next_key(struct cob* this, uint64_t key,
		bool *next_key_exists, uint64_t* next_key) {
	validate_key(key);

	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);

	// TODO: plain single FOR?
	// TODO: pointers for faster lookup? binsearch?
	for (uint64_t i = leaf_index * this->file.block_size;
			i < this->file.capacity; i++) {
		if (this->file.occupied[i] && this->file.items[i].key > key) {
			*next_key_exists = true;
			*next_key = this->file.items[i].key;
			return;
		}
	}
	*next_key_exists = false;
}

void cob_previous_key(struct cob* this, uint64_t key,
		bool *previous_key_exists, uint64_t* previous_key) {
	validate_key(key);

	// TODO: make this recursive instead
	uint64_t node_stack[50];
	uint64_t node_stack_size = 0;

	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index;
	veb_walk(this, key, node_stack, &node_stack_size, &leaf_index);
	ofm_range leaf = ofm_get_leaf(&this->file, leaf_index * this->file.block_size);

	// TODO: plain single FOR?
	// TODO: return pointer for faster lookup
	for (uint64_t i = 0; i < leaf.begin + leaf.size; i++) {
		uint64_t idx = leaf.begin + leaf.size - 1 - i;
		if (this->file.occupied[idx] &&
				this->file.items[idx].key < key) {
			*previous_key_exists = true;
			*previous_key = this->file.items[idx].key;
			return;
		}
	}
	*previous_key_exists = false;
}

void cob_init(struct cob* this) {
	ofm_init(&this->file);
	this->veb_minima = NULL;
	entirely_reset_veb(this);
}

void cob_destroy(struct cob this) {
	ofm_destroy(this.file);
	free(this.veb_minima);
}
