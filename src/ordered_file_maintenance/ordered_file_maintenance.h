#ifndef ORDERED_FILE_MAINTENANCE_H
#define ORDERED_FILE_MAINTENANCE_H

#include <stdbool.h>
#include <stdint.h>

#define NULL_INDEX 0xDEADBEEFDEADBEEF

struct {
	uint64_t reorganized_size;
} OFM_COUNTERS;

// The density of the entire structure is within [0.5;0.75].
typedef struct {
	uint64_t key;
	uint64_t value;
} ofm_item;

// struct parameters {
// 	uint64_t capacity;
// 	uint64_t block_size;
// };

//struct parameters adequate_parameters(uint64_t items);

typedef struct {
	// TODO: store as bitmap
	bool* occupied;

	uint64_t *keys;
	uint64_t *values;

	// ofm_item* items;
	//struct parameters parameters;
	uint64_t capacity;
	uint64_t block_size;
} ofm;

typedef struct {
	uint64_t begin;
	uint64_t size;

	ofm* file;
} ofm_range;

ofm_range ofm_get_leaf(ofm* file, uint64_t index);

void ofm_dump(ofm file);
void ofm_insert_before(ofm* file, ofm_item item, uint64_t insert_before_index,
		uint64_t *saved_at, ofm_range *touched_range);
void ofm_delete(ofm* file, uint64_t index, uint64_t *next_item_at,
		ofm_range *touched_range);
void ofm_init(ofm* file);
void ofm_destroy(ofm file);

bool ofm_is_entire_file(ofm_range block);
ofm_range ofm_block_parent(ofm_range block);

#endif
