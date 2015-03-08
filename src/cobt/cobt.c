#include "cobt/cobt.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "math/math.h"
#include "veb_layout/veb_layout.h"

#define NO_LOG_INFO
#include "log/log.h"

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
				range.file->keys[index] == key) {
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
			log_info("minimum [%" PRIu64 "+%" PRIu64 "]=%" PRIu64,
					range.begin, range.size,
					range.file->keys[index]);
			return range.file->keys[index];
		}
	}
	log_info("minimum [%" PRIu64 "+%" PRIu64 "]=infty",
			range.begin, range.size);
	return COB_INFINITY;
}

static ofm_range insert_sorted_order(ofm_range range, uint64_t key,
		uint64_t value) {
	bool found_after = false;
	uint64_t index_after;
	for (uint64_t i = range.begin; i < range.file->capacity; i++) {
		if (range.file->occupied[i] && range.file->keys[i] > key) {
			found_after = true;
			index_after = i;
			break;
		}
	}

	ofm_range touched_range;
	if (found_after) {
		log_info("inserting %" PRIu64 "=%" PRIu64 " before %" PRIu64,
				key, value, index_after);
		ofm_insert_before(range.file, key, &value,
				index_after, NULL, &touched_range);
	} else {
		ofm_insert_before(range.file, key, &value,
				range.file->capacity, NULL, &touched_range);
	}
	return touched_range;
}

static uint64_t leaf_range_count(cob this) {
	assert(this.file.capacity % this.file.block_size == 0);
	return this.file.capacity / this.file.block_size;
}

static uint64_t get_veb_height(cob this) {
	return exact_log2(leaf_range_count(this)) + 1;
}

static uint64_t veb_node_count(cob _this) {
	return (1 << get_veb_height(_this)) - 1;
}

static inline ofm_range left_half_of(ofm_range range) {
	return (ofm_range) {
		.begin = range.begin,
		.size = range.size / 2,
		.file = range.file
	};
}

static inline ofm_range right_half_of(ofm_range range) {
	return (ofm_range) {
		.begin = range.begin + range.size / 2,
		.size = range.size / 2,
		.file = range.file
	};
}

// TODO: remove recursive call
static void fix_range_recursive(cob* this, ofm_range range_to_fix,
		ofm_range current_range, struct drilldown_track* track) {
	log_info("to_fix=[%" PRIu64 "+%" PRIu64 "] current_range=[%" PRIu64 "+%" PRIu64 "] "
			" bs=%" PRIu64,
			range_to_fix.begin, range_to_fix.size,
			current_range.begin, current_range.size,
			this->file.block_size);
	const uint64_t current_nid = track->pos[track->depth];

	if (current_range.size > range_to_fix.size) {
		log_info("partially dirty");
		if (range_to_fix.begin <
				current_range.begin + current_range.size / 2) {
			drilldown_go_left(this->level_data, track);
			fix_range_recursive(this, range_to_fix,
					left_half_of(current_range), track);
		} else {
			drilldown_go_right(this->level_data, track);
			fix_range_recursive(this, range_to_fix,
					right_half_of(current_range), track);
		}
		if (this->veb_minima[track->pos[track->depth]] < this->veb_minima[current_nid]) {
			this->veb_minima[current_nid] = this->veb_minima[track->pos[track->depth]];
		}
		drilldown_go_up(track);
	} else if (current_range.size > this->file.block_size) {
		log_info("completely dirty");
		this->veb_minima[current_nid] = COB_INFINITY;
		drilldown_go_left(this->level_data, track);
		fix_range_recursive(this, range_to_fix,
				left_half_of(current_range), track);
		if (this->veb_minima[track->pos[track->depth]] < this->veb_minima[current_nid]) {
			this->veb_minima[current_nid] = this->veb_minima[track->pos[track->depth]];
		}
		drilldown_go_up(track);

		drilldown_go_right(this->level_data, track);
		fix_range_recursive(this, range_to_fix,
				right_half_of(current_range), track);
		if (this->veb_minima[track->pos[track->depth]] < this->veb_minima[current_nid]) {
			this->veb_minima[current_nid] = this->veb_minima[track->pos[track->depth]];
		}
		drilldown_go_up(track);
	} else {
		this->veb_minima[current_nid] = range_get_minimum(current_range);
	}
	log_info("=> %" PRIu64 " fixed to %" PRIu64,
			current_nid, this->veb_minima[current_nid]);
}

static void fix_range(cob* this, ofm_range range_to_fix) {
	struct drilldown_track track;
	drilldown_begin(&track);
	fix_range_recursive(this, range_to_fix, (ofm_range) {
		.begin = 0, .size = this->file.capacity, .file = &this->file
	}, &track);
}

static uint64_t veb_walk(const cob* this, uint64_t key) {
	log_info("veb_walking to key %" PRIu64, key);
	uint64_t stack_size = 0;

	struct drilldown_track track;
	drilldown_begin(&track);

	const uint64_t veb_height = get_veb_height(*this);
	// Walk down vEB layout to find where does the key belong.
	uint64_t leaf_index = 0;
	do {
		stack_size++;

		if (stack_size == veb_height) {
			log_info("-> %" PRIu64 " is the leaf we want", pointer);
			// This is the leaf.
			break;
		} else {
			// NOTE: this is the reason why right drilldowns
			// are more likely.
			drilldown_go_right(this->level_data, &track);
			if (key >= this->veb_minima[track.pos[track.depth]]) {
				// We want to go right.
				leaf_index = (leaf_index << 1) + 1;
			} else {
				drilldown_go_up(&track);
				drilldown_go_left(this->level_data, &track);
				leaf_index = leaf_index << 1;
			}
		}
	} while (true);

	return leaf_index;
}

static void entirely_reset_veb(cob* this) {
	// TODO: this realloc is probably wrong
	this->veb_minima = realloc(this->veb_minima,
			veb_node_count(*this) * sizeof(uint64_t));
	this->level_data = realloc(this->level_data,
			get_veb_height(*this) * sizeof(struct level_data));

	assert(this->veb_minima && this->level_data);

	veb_prepare(get_veb_height(*this), this->level_data);
	fix_range(this, (ofm_range) {
		.begin = 0,
		.size = this->file.capacity,
		.file = &this->file
	});
}

static void validate_key(uint64_t key) {
	CHECK(key != COB_INFINITY, "Trying to operate on key=COB_INFINITY");
}

static ofm_range get_leaf(cob* this, uint64_t leaf_index) {
	return ofm_get_leaf(&this->file, leaf_index * this->file.block_size);
}

int8_t cob_insert(cob* this, uint64_t key, uint64_t value) {
	validate_key(key);

	// Walk down vEB layout to find where does the key belong.
	const uint64_t leaf_index = veb_walk(this, key);
	if (range_find(get_leaf(this, leaf_index), key, NULL)) {
		// Duplicate key;
		return 1;
	}

	const struct parameters prior_parameters = get_params(this->file);

	// Insert into ordered file.
	const ofm_range reorg_range = insert_sorted_order(
			get_leaf(this, leaf_index), key, value);
	COB_COUNTERS.total_reorganized_size += reorg_range.size;
	log_info("reorged range [%" PRIu64 "+%" PRIu64 "]",
			reorg_range.begin, reorg_range.size);

	if (parameters_equal(get_params(this->file), prior_parameters)) {
		fix_range(this, reorg_range);
	} else {
		entirely_reset_veb(this);
	}
	return 0;
}

int8_t cob_delete(cob* this, uint64_t key) {
	validate_key(key);

	// Walk down vEB layout to find where does the key belong.
	const uint64_t leaf_index = veb_walk(this, key);

	const struct parameters prior_parameters = get_params(this->file);

	// Delete from ordered file.
	uint64_t index;
	if (!range_find(get_leaf(this, leaf_index), key, &index)) {
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

void cob_find(cob* this, uint64_t key, bool *found, uint64_t *value) {
	validate_key(key);

	// Walk down vEB layout to find where does the key belong.
	const uint64_t leaf_index = veb_walk(this, key);

	uint64_t index;
	if (range_find(get_leaf(this, leaf_index), key, &index)) {
		*found = true;
		if (value) {
			ofm_get_value(&this->file, index, value);
		}
	} else {
		*found = false;
	}
}

void cob_next_key(cob* this, uint64_t key,
		bool *next_key_exists, uint64_t* next_key) {
	validate_key(key);

	// Walk down vEB layout to find where does the key belong.
	const uint64_t leaf_index = veb_walk(this, key);

	// TODO: plain single FOR?
	// TODO: pointers for faster lookup? binsearch?
	for (uint64_t i = leaf_index * this->file.block_size;
			i < this->file.capacity; i++) {
		if (this->file.occupied[i] && this->file.keys[i] > key) {
			*next_key_exists = true;
			*next_key = this->file.keys[i];
			return;
		}
	}
	*next_key_exists = false;
}

void cob_previous_key(cob* this, uint64_t key,
		bool *previous_key_exists, uint64_t* previous_key) {
	validate_key(key);

	// Walk down vEB layout to find where does the key belong.
	const uint64_t leaf_index = veb_walk(this, key);
	ofm_range leaf = get_leaf(this, leaf_index);

	// TODO: plain single FOR?
	// TODO: return pointer for faster lookup
	for (uint64_t i = 0; i < leaf.begin + leaf.size; i++) {
		uint64_t idx = leaf.begin + leaf.size - 1 - i;
		if (this->file.occupied[idx] &&
				this->file.keys[idx] < key) {
			*previous_key_exists = true;
			*previous_key = this->file.keys[idx];
			return;
		}
	}
	*previous_key_exists = false;
}

void cob_init(cob* this) {
	// value size = sizeof(uint64_t)
	ofm_init(&this->file, sizeof(uint64_t));
	this->veb_minima = NULL;
	this->level_data = NULL;
	entirely_reset_veb(this);
}

void cob_destroy(cob this) {
	ofm_destroy(this.file);
	free(this.veb_minima);
	free(this.level_data);
}

void cob_check(cob* cob) {
	for (uint64_t i = 0; i < cob->file.capacity / cob->file.block_size;
			i++) {
		const uint64_t leaf_offset = i * cob->file.block_size;
		const uint64_t node = veb_get_leaf_number(i, get_veb_height(*cob));
		assert(cob->veb_minima[node] == range_get_minimum((ofm_range) {
				.begin = leaf_offset,
				.size = cob->file.block_size,
				.file = &cob->file
		}));
	}
}
