#ifndef ORDERED_FILE_MAINTENANCE_H
#define ORDERED_FILE_MAINTENANCE_H

#include <stdbool.h>
#include <stdint.h>

// Capacity (N) must be a power of two.
// Leaf range size = closest_pow2_floor(floor(log2(N))).
//
// Each leaf range is filled from the left.
//
// The density of the entire structure is within [0.5;0.75].

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

struct reorg_range {
	uint64_t begin;
	uint64_t size;
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

// Those functions return the touched range.
struct reorg_range ordered_file_insert_after(struct ordered_file file,
		uint64_t item, uint64_t insert_after_index);
struct reorg_range ordered_file_delete(struct ordered_file file,
		uint64_t index);

bool density_is_within_threshold(uint64_t slots_used, uint64_t slots_available,
		uint64_t depth, uint64_t structure_height);

#endif
