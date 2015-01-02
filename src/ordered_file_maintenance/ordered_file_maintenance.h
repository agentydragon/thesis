#ifndef ORDERED_FILE_MAINTENANCE_H
#define ORDERED_FILE_MAINTENANCE_H

#include <stdbool.h>
#include <stdint.h>

// The density of the entire structure is within [0.5;0.75].
typedef struct {
	uint64_t key;
	uint64_t value;
} ordered_file_item;

struct parameters {
	uint64_t capacity;
	uint64_t block_size;
};

struct parameters adequate_parameters(uint64_t items);

struct ordered_file {
	// TODO: store as bitmap
	bool* occupied;
	ordered_file_item* items;
	struct parameters parameters;
};

struct watched_index {
	uint64_t index;
	uint64_t* new_location;
};

struct ordered_file_range {
	uint64_t begin;
	uint64_t size;
};

// TODO: maybe relax requirements to allow easier implementation
uint64_t leaf_block_size(uint64_t capacity);
void range_insert_after(
		struct ordered_file file, struct ordered_file_range range,
		ordered_file_item inserted_item, uint64_t insert_after);
void range_compact(
		struct ordered_file file, struct ordered_file_range range,
		struct watched_index watched_index);
void range_spread_evenly(
		struct ordered_file file, struct ordered_file_range range,
		struct watched_index watched_index);
void range_describe(
		const struct ordered_file file, struct ordered_file_range range,
		char* buffer);

struct ordered_file_range get_leaf_range(
		struct ordered_file file, uint64_t index);

// Those functions return the touched range.
struct ordered_file_range ordered_file_insert_after(struct ordered_file* file,
		ordered_file_item item, uint64_t insert_after_index);
struct ordered_file_range ordered_file_insert_first(struct ordered_file* file,
		ordered_file_item item);
struct ordered_file_range ordered_file_delete(struct ordered_file* file,
		uint64_t index);

void ordered_file_init(struct ordered_file* file);
void ordered_file_destroy(struct ordered_file file);

bool global_density_within_threshold(uint64_t slots_used, uint64_t capacity);
bool density_is_within_threshold(uint64_t slots_used, uint64_t slots_available,
		uint64_t depth, uint64_t structure_height);


#endif
