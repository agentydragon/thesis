#ifndef ORDERED_FILE_MAINTENANCE_H
#define ORDERED_FILE_MAINTENANCE_H

#include <stdbool.h>
#include <stdint.h>

// Capacity (N) must be a power of two.
// Leaf range size = closest_pow2_floor(floor(log2(N))).
//
// Each leaf range is filled from the left.

struct ordered_file {
	// TODO: store as bitmap
	bool* occupied;
	uint64_t* contents;
	uint64_t capacity;
};

typedef uint64_t ordered_file_pointer;

// Conceptual.
struct subrange {
	bool* occupied;
	uint64_t* contents;
	uint64_t size;
};

struct watched_index {
	uint64_t index;
	uint64_t* new_location;
};

// TODO: maybe relax requirements to allow easier implementation
// TODO: change to subrange_insert_before_index to let me pass 0
bool subrange_insert_after(struct subrange subrange, uint64_t inserted_item,
		uint64_t insert_after);
void subrange_delete(struct subrange subrange, uint64_t item);
void subrange_compact(struct subrange subrange,
		struct watched_index watched_index);
void subrange_spread_evenly(struct subrange subrange,
		struct watched_index watched_index);
void subrange_describe(struct subrange subrange, char* buffer);

void ordered_file_insert_after(struct ordered_file file, uint64_t item,
		uint64_t insert_after_index);

bool density_is_within_threshold(uint64_t slots_used, uint64_t slots_available,
		uint64_t depth, uint64_t structure_height);

#endif

/*


// N elements (words) in specified order.
// Stored in array of linear size (got some blank spots).
// Updates: delete element / insert element (between 2 elements)
//
// (We can move elements around in interval of size O(lg^2 N)
// amortized, using O(1) scans ==> O((lg^2 N) / B) memory transfers.)

static uint64_t chunk_size(uint64_t structure_size) {
	return ceil_log2(structure_size);
}

static uint64_t chunk_depth(uint64_t structure_size) {
	return ceil_log2(ceil_div(structure_size, chunk_size));
	// TODO: log(N) - log(chunk_size)
}

// TODO: more rapid optimization (bucket insertions rewrite items and storages_occupied)
struct ordered_file {
	uint64_t *stored_below; // size = 2^(chunk_depth) - 1, in BFS order
	// (conceptually a tree)
	bool *storages_occupied; // N/chunk_size == size
	uint64_t *storage; // in chunk_size buckets; must be divisible
};

bool density_is_within_threshold(uint64_t slots_used, uint64_t slots_available,
		uint64_t depth, uint64_t structure_height) {
	// used / available >= 1/2 - (1/4) (depth / s_height)
	// used / available <= 3/4 + (1/4) (depth / s_height)

	// used >= available/2 - available/4 depth/s_height
	// 4*used >= 2*available - available depth/s_height
	// 4*used*s_height >= 2*available*s_height - available depth
	bool A = (4 * slots_used * structure_height >= 2 * slots_available * structure_height - slots_available * depth);

	// 4*used*s_height <= 3*available*s_height + available depth
	bool B = (4 * slots_used * structure_height <= 3 * slots_available * structure_height - slots_available * depth);
	return A && B;
}

void evenly_rebalance(uint64_t start, uint64_t end) {
	// TODO: evenly rebalance everything below [start,end]; rebuild stored_below
	TODO
}

void insert(uint64_t after_index) {
	// Reshuffle whole bucket.
	uint64_t first_free = -1;
	// TODO: optim, to je mask
	// TODO: optimizations for some case? (no need to rewrite whole chunk?)
	uint64_t this_chunk_index = after_index / chunk_size(this->structure_size);
	uint64_t this_chunk_start = this_chunk_index * chunk_size(this->structure_size);
	for (uint64_t i = 0, free = 0; i < chunk_size(this->structure_size); i++) {
		if (this->storage_occupied[i + this_chunk_start]) {
			if (free < i) {
				while (free < i && this->storage_occupied[free + this_chunk_start]) free++;

				if (free < i) {
					this->storage_occupied[free + this_chunk_start] = true;
					this->storage[free + this_chunk_start] = this->storage[i + this_chunk_start];
				}
			}
		}
	}

	const uint64_t structure_height = chunk_depth(this->structure_size);
	int64_t depth = chunk_depth(this->structure_size);
	uint64_t ptr_to_bt_node = exp2(depth) + this_chunk_index;
	uint64_t slots_available = chunk_size(this->structure_size);

	while (depth > 0 && !density_is_within_threshold(
				this->stored_below[ptr_to_bt_node],
				slots_available, depth, structure_height)) {
		depth--;
		slots_available *= 2;
		ptr_to_bt_node /= 2;
	}

	if (depth == 0) {
		// Reached up, but not resolvable.
		// Either too sparse, or too dense.
		// TODO: resize structure
		exit(1);
	}

	// Evenly rebalance elements.
	evenly_rebalance(TODO, TODO);
}
*/
