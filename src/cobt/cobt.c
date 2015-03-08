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

static ofm_range insert_sorted_order(cob* this, uint64_t index, uint64_t key,
		uint64_t value) {
	bool found_after = false;
	uint64_t index_after;
	for (uint64_t i = index; i < this->file.capacity; i++) {
		if (this->file.occupied[i] && this->file.keys[i] > key) {
			found_after = true;
			index_after = i;
			break;
		}
	}

	ofm_range touched_range;
	if (found_after) {
		log_info("inserting %" PRIu64 "=%" PRIu64 " before %" PRIu64,
				key, value, index_after);
		ofm_insert_before(&this->file, key, &value,
				index_after, NULL, &touched_range);
	} else {
		ofm_insert_before(&this->file, key, &value,
				this->file.capacity, NULL, &touched_range);
	}
	return touched_range;
}

static void fix_range(cob* this, ofm_range range_to_fix) {
	cobt_tree_refresh(&this->tree, (cobt_tree_range) {
		.begin = range_to_fix.begin,
		.end = range_to_fix.begin + range_to_fix.size
	});
}

static void entirely_reset_veb(cob* this) {
	cobt_tree_destroy(&this->tree);
	cobt_tree_init(&this->tree, this->file.keys, this->file.occupied,
			this->file.capacity);
}

static void validate_key(uint64_t key) {
	CHECK(key != COB_INFINITY, "Trying to operate on key=COB_INFINITY");
}

int8_t cob_insert(cob* this, uint64_t key, uint64_t value) {
	validate_key(key);

	const uint64_t index = cobt_tree_find_le(&this->tree, key);
	if (this->file.occupied[index] && this->file.keys[index] == key) {
		// Duplicate key;
		return 1;
	}

	const uint64_t prior_capacity = this->file.capacity;
	// Insert into ordered file.
	const ofm_range reorg_range = insert_sorted_order(this, index,
			key, value);
	COB_COUNTERS.total_reorganized_size += reorg_range.size;
	log_info("reorged range [%" PRIu64 "+%" PRIu64 "]",
			reorg_range.begin, reorg_range.size);

	if (this->file.capacity == prior_capacity) {
		fix_range(this, reorg_range);
	} else {
		entirely_reset_veb(this);
	}
	log_info("done inserting %" PRIu64 "=%" PRIu64, key, value);
	return 0;
}

int8_t cob_delete(cob* this, uint64_t key) {
	validate_key(key);

	log_info("delete %" PRIu64, key);

	// Walk down vEB layout to find where does the key belong.
	const uint64_t index = cobt_tree_find_le(&this->tree, key);

	if (!(this->file.occupied[index] && this->file.keys[index] == key)) {
		// Deleting nonexistant key.
		return 1;
	}

	const uint64_t prior_capacity = this->file.capacity;
	ofm_range reorg_range;
	ofm_delete(&this->file, index, NULL, &reorg_range);
	if (this->file.capacity == prior_capacity) {
		fix_range(this, reorg_range);
	} else {
		entirely_reset_veb(this);
	}
	return 0;
}

void cob_find(cob* this, uint64_t key, bool *found, uint64_t *value) {
	validate_key(key);
	const uint64_t index = cobt_tree_find_le(&this->tree, key);
	if (this->file.occupied[index] && this->file.keys[index] == key) {
		*found = true;
		if (value != NULL) {
			*value = *(uint64_t*) ofm_get_value(&this->file, index);
		}
	} else {
		*found = false;
	}
}

void cob_next_key(cob* this, uint64_t key,
		bool *next_key_exists, uint64_t* next_key) {
	validate_key(key);
	for (uint64_t i = cobt_tree_find_le(&this->tree, key);
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
	uint64_t start_index = cobt_tree_find_le(&this->tree, key);
	// TODO: plain single FOR?
	// TODO: return pointer for faster lookup
	for (uint64_t i = 0; i < start_index + 1; i++) {
		uint64_t idx = start_index - i;
		if (this->file.occupied[idx] && this->file.keys[idx] < key) {
			*previous_key_exists = true;
			*previous_key = this->file.keys[idx];
			return;
		}
	}
	*previous_key_exists = false;
}

void cob_init(cob* this) {
	// TODO: store chunks instead
	// value size = sizeof(uint64_t)
	ofm_init(&this->file, sizeof(uint64_t));
	cobt_tree_init(&this->tree, this->file.keys, this->file.occupied,
			this->file.capacity);
}

void cob_destroy(cob* this) {
	ofm_destroy(this->file);
	cobt_tree_destroy(&this->tree);
}
