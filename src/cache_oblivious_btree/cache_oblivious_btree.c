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
	log_info("looking for %" PRIu64 " in [%" PRIu64 "+%" PRIu64 "]",
			key, range.begin, range.size);
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

uint64_t cobt_range_get_minimum(ofm_range range) {
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
		log_info("inserting %" PRIu64 "=%" PRIu64 " before %" PRIu64,
				key, value, index_after);
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

uint64_t cobt_get_veb_height(struct cob this) {
	return exact_log2(leaf_range_count(this)) + 1;
}

uint64_t cobt_range_to_node_id(struct cob this, ofm_range range) {
	// TODO: optimize
	ofm_range whole = (ofm_range) {
		.begin = 0, .size = this.file.capacity
	};
	uint64_t node = 0;
	while (whole.begin != range.begin || whole.size != range.size) {
		// Is range on the left half?
		veb_pointer left, right;
		veb_get_children(node, cobt_get_veb_height(this),
				&left, &right);
		assert(left.present && right.present);
		if (range.begin < whole.begin + (whole.size / 2)) {
			// left
			node = left.node;
		} else {
			// right
			node = right.node;
			whole.begin += whole.size / 2;
		}
		whole.size /= 2;
	}
	return node;
}

static void fix_below(struct cob* this, ofm_range range_to_fix) {
	const uint64_t nid = cobt_range_to_node_id(*this, range_to_fix);
	this->veb_minima[nid] = cobt_range_get_minimum(range_to_fix);
	if (range_to_fix.size > this->file.block_size) {
		const ofm_range left = (ofm_range) {
			.begin = range_to_fix.begin,
			.size = range_to_fix.size / 2,
			.file = range_to_fix.file
		};
		const ofm_range right = (ofm_range) {
			.begin = range_to_fix.begin + range_to_fix.size / 2,
			.size = range_to_fix.size / 2,
			.file = range_to_fix.file
		};
		fix_below(this, left);
		fix_below(this, right);
	}
}

static void fix_range(struct cob* this, ofm_range range_to_fix) {
	// Fix all below first.
	fix_below(this, range_to_fix);
	while (!ofm_is_entire_file(range_to_fix)) {
		const uint64_t parent_nid = cobt_range_to_node_id(
				*this, ofm_block_parent(range_to_fix));
		veb_pointer left, right;
		veb_get_children(parent_nid, cobt_get_veb_height(*this),
				&left, &right);
		assert(left.present && right.present);
		const uint64_t old_min = this->veb_minima[parent_nid];
		if (this->veb_minima[left.node] < this->veb_minima[right.node]) {
			this->veb_minima[parent_nid] = this->veb_minima[left.node];
		} else {
			this->veb_minima[parent_nid] = this->veb_minima[right.node];
		}
		if (this->veb_minima[parent_nid] == old_min) {
			// Early abort. (Most of the time 1 fix is enough.)
			break;
		}
		range_to_fix = ofm_block_parent(range_to_fix);
	}
}

// TODO: veb_walk is a relic of the old cobt
static void veb_walk(const struct cob* this, uint64_t key,
		uint64_t* stack, uint64_t* _stack_size,
		uint64_t* _leaf_index) {
	log_info("veb_walking to key %" PRIu64, key);
	uint64_t stack_size = 0;

	const uint64_t veb_height = cobt_get_veb_height(*this);
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
	fix_range(this, (ofm_range) {
		.begin = 0,
		.size = this->file.capacity,
		.file = &this->file
	});
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
	COB_COUNTERS.total_reorganized_size += reorg_range.size;
	log_info("reorged range [%" PRIu64 "+%" PRIu64 "]",
			reorg_range.begin, reorg_range.size);

	if (parameters_equal(get_params(this->file), prior_parameters)) {
		fix_range(this, reorg_range);
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
	ofm_range leaf_sr = ofm_get_leaf(&this->file, leaf_index *
			this->file.block_size);
	uint64_t index;
	if (!range_find(leaf_sr, key, &index)) {
		// Deleting nonexistant key.
		return 1;
	}
	ofm_range reorg_range;
	ofm_delete(&this->file, index, NULL, &reorg_range);
	if (parameters_equal(get_params(this->file), prior_parameters)) {
		fix_range(this, reorg_range);
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
