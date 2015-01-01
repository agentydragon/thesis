#include "ordered_file_maintenance.h"
#include "../log/log.h"
#include "../math/math.h"

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INVALID_INDEX 0xDEADBEEFDEADBEEF
const struct watched_index NO_WATCH = {
	.index = INVALID_INDEX,
	.new_location = NULL
};

static uint64_t adequate_block_size(uint64_t capacity) {
	uint64_t x = floor_log2(capacity);
	// Because leafs must meet constraints, we need at least 4 slots.
	if (x < 4) {
		return 4;
	} else {
		return x;
	}
}

struct parameters adequate_parameters(uint64_t items) {
	const double multiplier = 1.25;
	const uint64_t max_acceptable_capacity = items * 2 - 1;
	const uint64_t min_acceptable_capacity = (items * 4) / 3 + 1;

	uint64_t capacity = pow(multiplier,
			ceil(log(items) / log(multiplier)) + 1);
	const uint64_t block_size = adequate_block_size(capacity);

	uint64_t block_count = capacity / block_size;
	if (capacity % block_size != 0) {
		block_count++;
	}

	capacity = block_size * block_count;
	// TODO: should need just O(1) adjustments
	while (capacity < min_acceptable_capacity) {
		capacity += block_size;
	}
	// TODO: should need just O(1) adjustments
	while (capacity > max_acceptable_capacity) {
		capacity -= block_size;
	}
	assert(capacity >= min_acceptable_capacity &&
			capacity <= max_acceptable_capacity);

	return (struct parameters) {
		.capacity = capacity,
		.block_size = block_size
	};
}

void range_describe(struct ordered_file file,
		struct ordered_file_range range, char* buffer) {
	for (uint64_t i = 0; i < range.size; i++) {
		if (i % 8 == 0 && i > 0) {
			buffer += sprintf(buffer, "\n");
		}
		if (file.occupied[range.begin + i]) {
			buffer += sprintf(buffer, "[%2" PRIu64 "]%4" PRIu64 " ",
					i, file.keys[range.begin + i]);
		} else {
			buffer += sprintf(buffer, "[%2" PRIu64 "]---- ", i);
		}
	}
}

static uint64_t get_leaf_depth(struct parameters parameters) {
	return ceil_log2(parameters.capacity / parameters.block_size);
}

// Shifts all elements of the range to the left.
void range_compact(
		struct ordered_file file, struct ordered_file_range range,
		struct watched_index watched_index) {
	for (uint64_t source = 0, target = 0; source < range.size; source++) {
		if (file.occupied[range.begin + source]) {
			file.occupied[range.begin + source] = false;
			while (file.occupied[range.begin + target]) target++;
			file.occupied[range.begin + target] = true;
			file.keys[range.begin + target] =
					file.keys[range.begin + source];
			if (source + range.begin == watched_index.index) {
				log_info("compact moves %" PRIu64 " to %" PRIu64,
						watched_index.index,
						target + range.begin);
				*(watched_index.new_location) = target + range.begin;
			}
		}
	}
}

static uint64_t range_get_occupied(struct ordered_file file,
		struct ordered_file_range range) {
	uint64_t occupied = 0;
	// TODO: optimize
	for (uint64_t i = 0; i < range.size; i++) {
		if (file.occupied[range.begin + i]) occupied++;
	}
	return occupied;
}

static bool range_is_full(struct ordered_file file,
		struct ordered_file_range range) {
	return range_get_occupied(file, range) == range.size;
}

static bool range_is_empty(struct ordered_file file,
		struct ordered_file_range range) {
	return range_get_occupied(file, range) == 0;
}

void range_insert_first(struct ordered_file file,
		struct ordered_file_range range, uint64_t inserted_item) {
	assert(!range_is_full(file, range));
	// TODO: optimize
	range_compact(file, range, NO_WATCH);
	const uint64_t occupied = range_get_occupied(file, range);
	for (uint64_t i = 0; i < occupied; i++) {
		const uint64_t to = range.begin + occupied - i,
				from = range.begin + occupied - i - 1;
		file.occupied[to] = file.occupied[from];
		file.keys[to] = file.keys[from];
	}
	file.occupied[range.begin] = true;
	file.keys[range.begin] = inserted_item;
}

void range_insert_after(struct ordered_file file,
		struct ordered_file_range range, uint64_t inserted_item,
		uint64_t insert_after_index) {
	assert(!range_is_full(file, range) &&
			!range_is_empty(file, range));
	range_compact(file, range, (struct watched_index) {
		.index = insert_after_index,
		.new_location = &insert_after_index
	});

	const uint64_t occupied = range_get_occupied(file, range);
	log_info("adding %" PRIu64 " after index %" PRIu64 ", "
			"occupied=%" PRIu64, inserted_item,
			insert_after_index, occupied);
	assert(insert_after_index - range.begin < occupied);
	for (uint64_t copy_from = occupied - 1; ; copy_from--) {
		const uint64_t to = range.begin + copy_from + 1,
				from = range.begin + copy_from;
		log_info("%" PRIu64 " <- %" PRIu64, to, from);
		file.occupied[to] = file.occupied[from];
		file.keys[to] = file.keys[from];

		if (from == insert_after_index) {
			break;
		}

		CHECK(copy_from != 0, "we are trying to copy to wrong places");
	}
	file.keys[insert_after_index + 1] = inserted_item;
}

void range_spread_evenly(struct ordered_file file,
		struct ordered_file_range range,
		struct watched_index watched_index) {
	uint64_t sublocation = INVALID_INDEX;
	struct watched_index subwatch = {
		.index = watched_index.index,
		.new_location = &sublocation
	};
	range_compact(file, range, subwatch);
	uint64_t elements = range_get_occupied(file, range);

	// TODO: avoid float arithmetic
	const float gap_size = ((float) range.size) / ((float) elements);
	for (uint64_t i = 0; i < elements; i++) {
		const uint64_t target_index =
				range.size - 1 - round(i * gap_size);

		const uint64_t from = range.begin + elements - 1 - i,
				to = range.begin + target_index;
		file.keys[to] = file.keys[from];
		file.occupied[from] = false;
		file.occupied[to] = true;
		if (sublocation == from) {
			log_info("even spread moves %" PRIu64 " to %" PRIu64,
					sublocation, to);
			*(watched_index.new_location) = to;
		}
	}
}

bool dense_enough(uint64_t slots_used, uint64_t slots_available,
		uint64_t depth, uint64_t structure_height) {
	// used >= available/2 - available/4 depth/s_height
	// 4*used >= 2*available - available depth/s_height
	// 4*used*s_height >= 2*available*s_height - available*depth
	// used / available >= 1/2 - (1/4) (depth / s_height)
	return (4 * slots_used * structure_height >=
			2 * slots_available * structure_height - slots_available * depth);
}

bool sparse_enough(uint64_t slots_used, uint64_t slots_available,
		uint64_t depth, uint64_t structure_height) {
	// used / available <= 3/4 + (1/4) (depth / s_height)
	// 4*used*s_height <= 3*available*s_height + available*depth
	return (4 * slots_used * structure_height <=
			3 * slots_available * structure_height - slots_available * depth);
}

bool density_is_within_threshold(uint64_t slots_used, uint64_t slots_available,
		uint64_t depth, uint64_t structure_height) {
	return dense_enough(slots_used, slots_available,
			depth, structure_height) &&
		sparse_enough(slots_used, slots_available,
			depth, structure_height);
}

/*
bool global_density_within_threshold(uint64_t slots_used, uint64_t capacity) {
	if (slots_used <= 4) {
		return capacity >= 2 * slots_used;
	}
	return density_is_within_threshold(slots_used, capacity, 0, 1);
}
*/

// Returns touched range.
static struct ordered_file_range reorganize(struct ordered_file* file,
		uint64_t index, struct watched_index watched_index) {
	uint64_t block_size = file->parameters.block_size;
	const uint64_t leaf_depth = get_leaf_depth(file->parameters);
	uint64_t depth = leaf_depth;

	// TODO: interspersed left-right scan
	while (block_size <= file->parameters.capacity) {
		uint64_t block_offset = (index / block_size) * block_size;

		assert(block_offset < file->parameters.capacity &&
				block_offset + block_size <= file->parameters.capacity);
		assert(file->parameters.capacity % block_size == 0);

		struct ordered_file_range block = {
			.begin = block_offset,
			.size = block_size
		};
		const uint64_t occupied = range_get_occupied(*file, block);
		if (density_is_within_threshold(
				occupied, block_size, depth, leaf_depth)) {
			range_spread_evenly(*file, block, watched_index);
			return (struct ordered_file_range) {
				.begin = block_offset,
				.size = block_size
			};
		} else {
			log_info("height=%" PRIu64 ", depth=%" PRIu64 ": "
					"range (%" PRIu64 "...%" PRIu64 ") "
					"not within threshold "
					"(%" PRIu64 "/%" PRIu64"=%lf)",
					leaf_depth, depth,
					block_offset, block_offset + block_size,
					occupied, block_size,
					((double) occupied) / block_size);
		}
		block_size *= 2;
		depth--;
	}

	// Nova struktura musi mit hustotu 1/2 az 3/4.
	log_fatal("ordered file maintenance structure "
			"does not meet density bounds. TODO: resize to fit.");
}

static struct ordered_file_range get_leaf_range_for(struct ordered_file file,
		uint64_t index) {
	return get_leaf_range(file, index / file.parameters.block_size);
}

struct ordered_file_range get_leaf_range(struct ordered_file file,
		uint64_t leaf_number) {
	const uint64_t leaf_size = file.parameters.block_size;

	return (struct ordered_file_range) {
		.begin = leaf_number * leaf_size,
		.size = leaf_size
	};
}

struct ordered_file_range ordered_file_insert_first(struct ordered_file* file,
		uint64_t item) {
	struct ordered_file_range reorg_range = reorganize(file, 0, NO_WATCH);
	struct ordered_file_range leaf = get_leaf_range_for(*file, 0);
	CHECK(!range_is_full(*file, leaf), "reorganization didn't help");
	range_insert_first(*file, leaf, item);
	return reorg_range;
}

struct ordered_file_range ordered_file_insert_after(struct ordered_file* file,
		uint64_t item, uint64_t insert_after_index) {
	struct ordered_file_range reorg_range = reorganize(file,
			insert_after_index,
			(struct watched_index) {
				.index = insert_after_index,
				.new_location = &insert_after_index
			});
	struct ordered_file_range leaf_range = get_leaf_range_for(*file,
			insert_after_index);
	CHECK(!range_is_full(*file, leaf_range), "reorganization didn't help");
	range_insert_after(*file, leaf_range, item, insert_after_index);
	return reorg_range;
}

struct ordered_file_range ordered_file_delete(struct ordered_file* file,
		uint64_t index) {
	assert(file->occupied[index]);
	file->occupied[index] = false;
	return reorganize(file, index, NO_WATCH);
}
