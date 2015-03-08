#include "ofm/ofm.h"

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log/log.h"
#include "math/math.h"

/*
void range_describe(ofm file, ofm_range range, char* buffer) {
	for (uint64_t i = 0; i < range.size; i++) {
		if (i % 8 == 0 && i > 0) {
			buffer += sprintf(buffer, "\n");
		}
		if (file.occupied[range.begin + i]) {
			buffer += sprintf(buffer, "[%2" PRIu64 "]%4" PRIu64 " ",
					i, file.items[range.begin + i].key);
		} else {
			buffer += sprintf(buffer, "[%2" PRIu64 "]---- ", i);
		}
	}
}
*/

static void alloc_file(ofm* file) {
	assert(!file->keys && !file->values && !file->occupied);
	file->keys = calloc(file->capacity, sizeof(uint64_t));
	file->values = calloc(file->capacity, file->value_size);
	file->occupied = calloc(file->capacity, sizeof(bool));
	assert(file->keys && file->values && file->occupied);
	for (uint64_t i = 0; i < file->capacity; i++) {
		file->occupied[i] = false;
	}
}

static void* value_address(ofm* file, uint64_t index) {
	return file->values + file->value_size * index;
}

static void assign_value(ofm* file, uint64_t index, const void* source) {
	void* target = value_address(file, index);
	if (target != source) {
		memcpy(target, source, file->value_size);
	}
}

static void copy_item(ofm* target, uint64_t target_index,
		ofm* source, uint64_t source_index) {
	assert(source->value_size == target->value_size);
	target->keys[target_index] = source->keys[source_index];
	assign_value(target, target_index, value_address(source, source_index));
}

static char buffer[65536];

void ofm_dump(ofm file) {
	strcpy(buffer, "");
	uint64_t z = 0;
	for (uint64_t i = 0; i < file.capacity; i++) {
		if (i % file.block_size == 0) {
			z += sprintf(buffer + z, "\n    ");
		}
		if (file.occupied[i]) {
			z += sprintf(buffer + z, "%4" PRIu64 " ",
					file.keys[i]);
		} else {
			z += sprintf(buffer + z, ".... ");
		}
	}
	log_info("%s", buffer);
}

struct parameters { uint64_t capacity, block_size; };

struct parameters adequate_parameters(uint64_t items) {
	if (items < 4) {
		return (struct parameters) {
			.capacity = 4,
			.block_size = 4
		};
	}

	uint64_t multiplier, capacity;
	for (uint64_t i = 0; ; i++) {
		switch (i % 3) {
		case 0:
			multiplier = 4;
			break;
		case 1:
			multiplier = 5;
			break;
		case 2:
			multiplier = 6;
			break;
		default:
			log_fatal("bad");
		}

		capacity = multiplier * 1ULL << (i / 3);
		const double density = ((double) items) / capacity;

		// Global density bounds are [0.5;0.75], this is stricter,
		// so at least O(N) items will be inserted or deleted
		// before next resize.
		// (We could probably say [0.55;0.7] here, but that doesn't
		// work for very small sizes... TODO: clean up?)
		if (density >= 0.52 && density <= 0.7) {
			// Great, this capacity will fit!
			break;
		}

		if (density < 0.5) {
			log_fatal("we will probably not find it :(");
		}
	}

	uint64_t block_size = multiplier;
	while (block_size * 2 <= floor_log2(capacity)) block_size *= 2;

	return (struct parameters) {
		.capacity = capacity,
		.block_size = block_size
	};
}

ofm_range ofm_get_leaf(ofm* file, uint64_t index) {
	return (ofm_range) {
		.file = file,
		.begin = index - (index % file->block_size),
		.size = file->block_size
	};
}

static bool ofm_block_full(ofm_range block) {
	for (uint64_t i = block.begin; i < block.begin + block.size; i++) {
		if (!block.file->occupied[i]) return false;
	}
	return true;
}

ofm_range ofm_block_parent(ofm_range block) {
	block.size *= 2;
	block.begin -= block.begin % block.size;
	return block;
}

bool ofm_is_entire_file(ofm_range block) {
	return block.begin == 0 && block.size == block.file->capacity;
}

static void ofm_move(ofm file, uint64_t to, uint64_t from, uint64_t *watch) {
	log_verbose(4, "%" PRIu64 " <- %" PRIu64, to, from);
	assert(to < file.capacity && from < file.capacity);
	if (watch != NULL && *watch == from) {
		*watch = to;
	}
	if (file.occupied[from]) {
		copy_item(&file, to, &file, from);
		file.occupied[from] = false;
		file.occupied[to] = true;
	}
}

static void ofm_compact_left(ofm_range block, uint64_t *watch) {
	log_verbose(2, "compact_left(%" PRIu64 "+%" PRIu64 ")",
			block.begin, block.size);
	uint64_t to = block.begin;
	for (uint64_t i = 0; i < block.size; i++) {
		uint64_t from = block.begin + i;
		if (block.file->occupied[from]) {
			ofm_move(*block.file, to++, from, watch);
		}
	}
	IF_LOG_VERBOSE(3) {
		ofm_dump(*block.file);
	}
}

static void ofm_compact_right(ofm_range block, uint64_t *watch) {
	log_verbose(2, "compact_right[%" PRIu64 "+%" PRIu64 "]",
			block.begin, block.size);
	uint64_t to = block.begin + block.size - 1;
	for (uint64_t i = 0; i < block.size; i++) {
		uint64_t from = block.begin + block.size - 1 - i;
		assert(from <= to);
		if (block.file->occupied[from]) {
			log_verbose(3, "i=%" PRIu64 " "
					"to=%" PRIu64 " from=%" PRIu64,
					i, to, from);
			ofm_move(*block.file, to--, from, watch);
		}
	}
	IF_LOG_VERBOSE(3) {
		ofm_dump(*block.file);
	}
}

static uint64_t ofm_count_occupied(ofm_range block) {
	uint64_t occupied = 0;
	for (uint64_t i = block.begin; i < block.begin + block.size; i++) {
		if (block.file->occupied[i]) {
			++occupied;
		}
	}
	return occupied;
}

static void ofm_spread(ofm_range block, uint64_t *watch) {
	log_verbose(2, "ofm_spread");
	ofm_compact_right(block, watch);
	const uint64_t occupied = ofm_count_occupied(block);
	if (occupied > 0) {
		const double gap = ((double) block.size) / occupied;
		for (uint64_t i = 0; i < occupied; i++) {
			uint64_t from = block.begin + block.size - occupied + i;
			uint64_t to = block.begin + floor(gap * i);
			assert(to <= from);
			ofm_move(*block.file, to, from, watch);
		}
	}
}

static bool ofm_block_within_threshold(ofm_range block) {
	uint64_t leaf_depth = exact_log2(
			block.file->capacity / block.file->block_size);
	if (leaf_depth == 0) leaf_depth = 1;  // avoid 0 division
	const uint64_t block_depth = leaf_depth - exact_log2(
			block.size / block.file->block_size);

	const double min_density = 1./2 -
		((double) block_depth / leaf_depth) / 4;
	const double max_density = 3./4 +
		((double) block_depth / leaf_depth) / 4;
	const double density = ((double) ofm_count_occupied(block)) /
		block.size;
	log_verbose(2, "bs=%" PRIu64 " leaf_size=%" PRIu64 " d=%lf "
			"min_d=%lf max_d=%lf", block.size,
			block.file->block_size, density, min_density,
			max_density);
	return min_density <= density && density <= max_density;
}

static void rebalance(ofm* file, ofm_range start_block,
		ofm_range *touched_block, uint64_t *watch) {
	ofm_range range = start_block;

	while (!ofm_is_entire_file(range) &&
			!ofm_block_within_threshold(range)) {
		range = ofm_block_parent(range);
	}

	if (ofm_is_entire_file(range)) {
		log_verbose(1, "entire file out of balance");
		struct parameters parameters = adequate_parameters(
				ofm_count_occupied(range));

		if (parameters.capacity == file->capacity &&
				parameters.block_size == file->block_size) {
			log_verbose(1, "ignoring, parameters are "
					"still adequate.");
		} else {
			ofm new_file = {
				.block_size = parameters.block_size,
				.capacity = parameters.capacity,
				.value_size = file->value_size,
				.keys = NULL,
				.values = NULL,
				.occupied = NULL,
			};
			alloc_file(&new_file);
			for (uint64_t i = 0, j = 0; i < file->capacity; i++) {
				if (file->occupied[i]) {
					if (watch != NULL && *watch == i) {
						*watch = j;
					}
					new_file.occupied[j] = true;
					copy_item(&new_file, j, file, i);
					j++;
				}
			}
			ofm_destroy(*file);
			*file = new_file;

			const ofm_range whole_file = {
				.begin = 0,
				.size = file->capacity,
				.file = file,
			};
			log_verbose(1, "resized, new capacity=%" PRIu64 " "
					"occupied=%" PRIu64, file->capacity,
					ofm_count_occupied(whole_file));
			range = whole_file;
			assert(ofm_block_within_threshold(range));
		}
	} else {
		assert(ofm_block_within_threshold(range));
	}
	OFM_COUNTERS.reorganized_size += range.size;

	ofm_spread(range, watch);
	if (touched_block != NULL) {
		*touched_block = range;
	}
}

// Moves every item in the block, starting with index `step_start`,
// one slot to the right.
void ofm_step_right(ofm_range block, uint64_t step_start) {
	log_verbose(2, "ofm_step_right");
	// There needs to be some free space at the end.
	for (uint64_t i = block.begin + block.size - 1; i > step_start; i--) {
		assert(!block.file->occupied[i]);
		ofm_move(*block.file, i, i - 1, NULL);
	}
}

void* ofm_get_value(ofm* file, uint64_t index) {
	return value_address(file, index);
}

void ofm_insert_before(ofm* file, uint64_t key, const void* value,
		uint64_t insert_before_index, uint64_t *saved_at,
		ofm_range *touched_range) {
	// log_verbose(2, "ofm_insert_before(%" PRIu64 "=%" PRIu64 ", "
	// 		"before_index=%" PRIu64 ")", item.key, item.value,
	// 		insert_before_index);
	assert(insert_before_index <= file->capacity);

	ofm_range block;
	if (insert_before_index == file->capacity) {
		// Special case. We will shift everything to the left,
		// and then insert to the end.
		block = ofm_get_leaf(file, file->capacity - 1);
	} else {
		// We will shift everything to the left,
		// then shift everything after `insert_before_index`
		// by 1 to the right and reshuffle.
		assert(file->occupied[insert_before_index]);
		block = ofm_get_leaf(file, insert_before_index);
	}

	while (ofm_block_full(block) && !ofm_is_entire_file(block)) {
		block = ofm_block_parent(block);
	}

	if (ofm_block_full(block) && ofm_is_entire_file(block)) {
		bool was_end = (insert_before_index == file->capacity);
		rebalance(file, (ofm_range) {
				.begin = 0,
				.size = file->capacity,
				.file = file }, NULL, &insert_before_index);
		if (was_end) insert_before_index = file->capacity;
		ofm_insert_before(file, key, value,
				insert_before_index, saved_at,
				touched_range);
		return;
		//log_fatal("File full.");
	}

	assert(!ofm_block_full(block));
	ofm_compact_left(block, &insert_before_index);

	if (insert_before_index == file->capacity) {
		--insert_before_index;
	} else {
		assert(insert_before_index >= block.begin &&
				insert_before_index < block.begin + block.size);
		ofm_step_right(block, insert_before_index);
	}
	assert(!file->occupied[insert_before_index]);
	file->occupied[insert_before_index] = true;
	file->keys[insert_before_index] = key;
	assign_value(file, insert_before_index, value);
	if (saved_at != NULL) {
		*saved_at = insert_before_index;
	}

	rebalance(file, block, touched_range, saved_at);
	IF_LOG_VERBOSE(3) {
		ofm_dump(*file);
	}
}

// `next_item_at` will hold the position where the next item was moved,
// or NULL_INDEX if there is no such item.
void ofm_delete(ofm* file, uint64_t index, uint64_t *next_item_at,
		ofm_range *touched_range) {
	assert(file->occupied[index]);
	uint64_t next_index = NULL_INDEX;

	for (uint64_t i = index + 1; i < file->capacity; i++) {
		if (file->occupied[i]) {
			next_index = i;
			break;
		}
	}

	file->occupied[index] = false;
	if (next_item_at != NULL) {
		*next_item_at = next_index;
	}
	rebalance(file, ofm_get_leaf(file, index), touched_range, next_item_at);
}

void ofm_init(ofm* file, size_t value_size) {
	// TODO: copy over?
	// TODO: merge with new_ordered_file
	file->capacity = 4;
	file->block_size = 4;
	file->value_size = value_size;
	file->keys = file->values = file->occupied = NULL;
	alloc_file(file);
}

void ofm_destroy(ofm file) {
	free(file.keys);
	free(file.values);
	free(file.occupied);
}
