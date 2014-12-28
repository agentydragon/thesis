#include "ordered_file_maintenance.h"
#include "../log/log.h"
#include "../math/math.h"

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const uint64_t INVALID_INDEX = 0xDEADBEEFDEADBEEF;

void subrange_describe(struct subrange subrange, char* buffer) {
	for (uint64_t i = 0; i < subrange.size; i++) {
		if (i % 4 == 0 && i > 0) {
			buffer += sprintf(buffer, "\n");
		}
		if (subrange.occupied[i]) {
			buffer += sprintf(buffer, "[%2" PRIu64 "]%4" PRIu64 " ",
					i, subrange.contents[i]);
		} else {
			buffer += sprintf(buffer, "[%2" PRIu64 "]---- ", i);
		}
	}
}

uint64_t subrange_leaf_size(uint64_t capacity) {
	uint64_t x = closest_pow2_floor(floor_log2(capacity));
	// Because subranges must meet constraints, we need at least 4 slots.
	if (x < 4) {
		return 4;
	} else {
		return x;
	}
}

uint64_t leaf_depth(uint64_t capacity) {
	return floor_log2(capacity / subrange_leaf_size(capacity));
}

// Shifts all elements of the subrange to the left.
void subrange_compact(struct subrange subrange,
		struct watched_index watched_index) {
	for (uint64_t source = 0, target = 0; source < subrange.size;
			source++) {
		if (subrange.occupied[source]) {
			subrange.occupied[source] = false;
			while (subrange.occupied[target]) target++;
			subrange.occupied[target] = true;
			subrange.contents[target] = subrange.contents[source];
			if (source == watched_index.index) {
				*(watched_index.new_location) = target;
			}
		}
	}
}

static uint64_t subrange_get_occupied(struct subrange subrange) {
	uint64_t occupied = 0;
	// TODO: optimize
	for (uint64_t i = 0; i < subrange.size; i++) {
		if (subrange.occupied[i]) occupied++;
	}
	return occupied;
}

static bool subrange_find(struct subrange subrange, uint64_t item,
		uint64_t *index) {
	for (uint64_t i = 0; i < subrange.size; i++) {
		if (subrange.occupied[i] && subrange.contents[i] == item) {
			*index = i;
			return true;
		}
	}
	return false;
}

// Returns false if the subrange is full. In that case, we need to rebalance.
bool subrange_insert_after(struct subrange subrange, uint64_t inserted_item,
		uint64_t insert_after) {
	subrange_compact(subrange, (struct watched_index) {
		.index = INVALID_INDEX,
		.new_location = NULL
	});

	uint64_t occupied = subrange_get_occupied(subrange);
	uint64_t index_in_subrange;
	if (!subrange_find(subrange, insert_after, &index_in_subrange)) {
		log_fatal("element to insert after (%" PRIu64 ") is not "
				"in this subrange", insert_after);
		return false;
	}
	if (subrange.size == occupied) {
		// Subrange is full.
		return false;
	} else {
		for (uint64_t copy_from = occupied - 1; ; copy_from--) {
			subrange.occupied[copy_from + 1] = subrange.occupied[copy_from];
			subrange.contents[copy_from + 1] = subrange.contents[copy_from];

			if (copy_from == index_in_subrange) {
				break;
			}
		}
		subrange.contents[index_in_subrange + 1] = inserted_item;
		return true;
	}
}

void subrange_delete(struct subrange subrange, uint64_t deleted_item) {
	uint64_t index;
	if (!subrange_find(subrange, deleted_item, &index)) {
		log_fatal("element to delete (%" PRIu64 ") "
				"not found in subrange", deleted_item);
	} else {
		subrange.occupied[index] = false;
	}
}

void subrange_spread_evenly(struct subrange subrange,
		struct watched_index watched_index) {
	uint64_t sublocation = INVALID_INDEX;
	struct watched_index subwatch = {
		.index = watched_index.index,
		.new_location = &sublocation
	};
	subrange_compact(subrange, subwatch);
	uint64_t elements = subrange_get_occupied(subrange);
	// TODO: avoid float arithmetic

	const float gap_size = ((float) subrange.size) / ((float) elements);
	for (uint64_t i = 0; i < elements; i++) {
		const uint64_t target_index =
				subrange.size - 1 - round(i * gap_size);
		subrange.contents[target_index] = subrange.contents[elements - 1 - i];
		subrange.occupied[elements - 1 - i] = false;
		subrange.occupied[target_index] = true;
		if (sublocation == elements - 1 - i) {
			log_info("%" PRIu64 " moves to %" PRIu64, sublocation,
					target_index);
			*(watched_index.new_location) = target_index;
		}
	}
}

bool density_is_within_threshold(uint64_t slots_used, uint64_t slots_available,
		uint64_t depth, uint64_t structure_height) {
	// used / available >= 1/2 - (1/4) (depth / s_height)
	// used / available <= 3/4 + (1/4) (depth / s_height)

	// used >= available/2 - available/4 depth/s_height
	// 4*used >= 2*available - available depth/s_height
	// 4*used*s_height >= 2*available*s_height - available*depth
	const bool A = (4 * slots_used * structure_height >=
			2 * slots_available * structure_height - slots_available * depth);

	// 4*used*s_height <= 3*available*s_height + available*depth
	const bool B = (4 * slots_used * structure_height <=
			3 * slots_available * structure_height - slots_available * depth);
	return A && B;
}

// Returns touched range.
static struct reorg_range reorganize(struct ordered_file file, uint64_t index, struct watched_index watched_index) {
	const uint64_t leaf_size = subrange_leaf_size(file.capacity);
	uint64_t block_size = leaf_size;
	uint64_t depth = leaf_depth(file.capacity);
	uint64_t block_offset;

	while (block_size <= file.capacity) {
		block_offset = (index / block_size) * leaf_size;

		struct subrange block_subrange = {
			.occupied = file.occupied + block_offset,
			.contents = file.contents + block_offset,
			.size = block_size
		};
		if (density_is_within_threshold(
				subrange_get_occupied(block_subrange),
				block_size, depth, leaf_depth(file.capacity))) {
			subrange_spread_evenly(block_subrange, watched_index);
			return (struct reorg_range) {
				.begin = block_offset,
				.size = block_size
			};
		}
		block_size *= 2;
		depth--;
	}

	log_fatal("ordered file maintenance structure "
			"does not meet density bounds. TODO: resize to fit.");
	return (struct reorg_range) {
		.begin = INVALID_INDEX,
		.size = INVALID_INDEX
	};
}

static struct subrange get_leaf_subrange_for(struct ordered_file file, uint64_t index) {
	const uint64_t leaf_size = subrange_leaf_size(file.capacity);
	return get_leaf_subrange(file, index / leaf_size);
}

struct subrange get_leaf_subrange(struct ordered_file file, uint64_t leaf_number) {
	const uint64_t leaf_size = subrange_leaf_size(file.capacity);
	const uint64_t leaf_offset = leaf_number * leaf_size;

	return (struct subrange) {
		.occupied = file.occupied + leaf_offset,
		.contents = file.contents + leaf_offset,
		.size = leaf_size
	};
}

struct reorg_range ordered_file_insert_after(struct ordered_file file,
		uint64_t item, uint64_t insert_after_index) {
	// TODO: perhaps we should reorg when the block stops meeting
	// density conditions?
	struct subrange leaf_subrange = get_leaf_subrange_for(file,
			insert_after_index);
	if (subrange_insert_after(leaf_subrange, item,
				file.contents[insert_after_index])) {
		// All is good.
		return (struct reorg_range) {
			// TODO: hack
			.begin = leaf_subrange.contents - file.contents,
			.size = leaf_subrange.size
		};
	}

	log_info("we will need to reorganize");
	struct reorg_range reorg_range = reorganize(file, insert_after_index,
			(struct watched_index) {
				.index = insert_after_index,
				.new_location = &insert_after_index
			});
	// TODO
	if (!subrange_insert_after(get_leaf_subrange_for(file, insert_after_index),
				item, file.contents[insert_after_index])) {
		log_fatal("reorganization didn't help");
	}
	return reorg_range;
}

struct reorg_range ordered_file_delete(struct ordered_file file,
		uint64_t index) {
	// TODO: perhaps we should reorg when the block stops meeting
	// density conditions?
	assert(file.occupied[index]);
	struct subrange leaf = get_leaf_subrange_for(file, index);
	subrange_delete(leaf, file.contents[index]);
	if (subrange_get_occupied(leaf) == 0) {
		return reorganize(file, index, (struct watched_index) {
			.index = INVALID_INDEX,
			.new_location = NULL
		});
	} else {
		return (struct reorg_range) {
			// TODO: hack
			.begin = leaf.contents - file.contents,
			.size = leaf.size
		};
	}
}
