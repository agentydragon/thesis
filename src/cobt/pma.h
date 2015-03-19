#ifndef COBT_PMA_H
#define COBT_PMA_H

#include <stdbool.h>
#include <stdint.h>

struct {
	uint64_t reorganized_size;
} PMA_COUNTERS;

// Stores an ordered list with uint64_t keys and void* values (typedefed
// as pma_value).
// The density of the entire structure is within [0.5;0.75].

typedef void* pma_value;

typedef struct {
	// TODO: store as bitmap
	bool* occupied;
	uint64_t* keys;
	pma_value* values;

	uint64_t capacity;
	uint64_t block_size;
} pma;

typedef struct {
	uint64_t begin;
	uint64_t size;

	pma* file;
} pma_range;

void pma_dump(pma file);
pma_range pma_insert_before(pma* file, uint64_t key, pma_value value,
		uint64_t insert_before_index);
pma_range pma_delete(pma* file, uint64_t index);
void pma_init(pma* file);
void pma_destroy(pma* file);

pma_value pma_get_value(pma* file, uint64_t index);

typedef struct {
	pma* file;
	uint64_t scratch;
	uint64_t allowed_capacity;
} pma_stream;

void pma_stream_start(pma* file, uint64_t size, pma_stream *stream);
void pma_stream_push(pma_stream* stream, uint64_t key, pma_value value);

#endif
