#include "cobt/pma.h"

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log/log.h"
#include "math/math.h"

/*
void range_describe(pma file, pma_range range, char* buffer) {
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

static void alloc_file(pma* file) {
	assert(!file->keys && !file->values && !file->occupied);
	file->keys = calloc(file->capacity, sizeof(uint64_t));
	if (file->keys == NULL) {
		log_fatal("couldn't allocate %" PRIu64 " uint64 keys for pma",
			file->capacity);
	}
	file->values = calloc(file->capacity, sizeof(pma_value));
	if (file->values == NULL) {
		log_fatal("couldn't allocate %" PRIu64 " values for pma",
				file->capacity);
	}
	file->occupied = calloc(file->capacity, sizeof(bool));
	if (file->occupied == NULL) {
		log_fatal("couldn't allocate %" PRIu64 " bools for pma",
				file->capacity);
	}
}

static void copy_item(pma* target, uint64_t target_index,
		pma* source, uint64_t source_index) {
	target->keys[target_index] = source->keys[source_index];
	target->values[target_index] = source->values[source_index];
}

void pma_dump(pma file) {
	char buffer[65536];
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

static struct parameters adequate_parameters(uint64_t items) {
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

static pma_range get_leaf(pma* file, uint64_t index) {
	return (pma_range) {
		.file = file,
		.begin = index - (index % file->block_size),
		.size = file->block_size
	};
}

static bool pma_block_full(pma_range block) {
	for (uint64_t i = block.begin; i < block.begin + block.size; i++) {
		if (!block.file->occupied[i]) return false;
	}
	return true;
}

static pma_range block_parent(pma_range block) {
	block.size *= 2;
	block.begin -= block.begin % block.size;
	return block;
}

static bool is_entire_file(pma_range block) {
	return block.begin == 0 && block.size == block.file->capacity;
}

static void pma_move(pma file, uint64_t to, uint64_t from, uint64_t *watch) {
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

static void pma_compact_left(pma_range block, uint64_t *watch) {
	log_verbose(2, "compact_left(%" PRIu64 "+%" PRIu64 ")",
			block.begin, block.size);
	uint64_t to = block.begin;
	for (uint64_t i = 0; i < block.size; i++) {
		uint64_t from = block.begin + i;
		if (block.file->occupied[from]) {
			pma_move(*block.file, to++, from, watch);
		}
	}
	IF_LOG_VERBOSE(3) {
		pma_dump(*block.file);
	}
}

static void pma_compact_right(pma_range block, uint64_t *watch) {
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
			pma_move(*block.file, to--, from, watch);
		}
	}
	IF_LOG_VERBOSE(3) {
		pma_dump(*block.file);
	}
}

static uint64_t pma_count_occupied(pma_range block) {
	uint64_t occupied = 0;
	for (uint64_t i = block.begin; i < block.begin + block.size; i++) {
		if (block.file->occupied[i]) {
			++occupied;
		}
	}
	return occupied;
}

static void pma_spread(pma_range block, uint64_t *watch) {
	log_verbose(2, "pma_spread");
	pma_compact_right(block, watch);
	const uint64_t occupied = pma_count_occupied(block);
	if (occupied > 0) {
		const double gap = ((double) block.size) / occupied;
		for (uint64_t i = 0; i < occupied; i++) {
			uint64_t from = block.begin + block.size - occupied + i;
			uint64_t to = block.begin + floor(gap * i);
			assert(to <= from);
			pma_move(*block.file, to, from, watch);
		}
	}
}

static bool pma_block_within_threshold(pma_range block) {
	uint64_t leaf_depth = exact_log2(
			block.file->capacity / block.file->block_size);
	if (leaf_depth == 0) leaf_depth = 1;  // avoid 0 division
	const uint64_t block_depth = leaf_depth - exact_log2(
			block.size / block.file->block_size);

	const double min_density = 1./2 -
		((double) block_depth / leaf_depth) / 4;
	const double max_density = 3./4 +
		((double) block_depth / leaf_depth) / 4;
	const double density = ((double) pma_count_occupied(block)) /
		block.size;
	log_verbose(2, "bs=%" PRIu64 " leaf_size=%" PRIu64 " d=%lf "
			"min_d=%lf max_d=%lf", block.size,
			block.file->block_size, density, min_density,
			max_density);
	return min_density <= density && density <= max_density;
}

static pma_range rebalance(pma* file, pma_range start_block, uint64_t *watch) {
	pma_range range = start_block;

	while (!is_entire_file(range) &&
			!pma_block_within_threshold(range)) {
		range = block_parent(range);
	}

	if (is_entire_file(range)) {
		log_verbose(1, "entire file out of balance");
		const uint64_t count = pma_count_occupied(range);
		struct parameters parameters = adequate_parameters(count);

		if (parameters.capacity == file->capacity &&
				parameters.block_size == file->block_size) {
			log_verbose(1, "ignoring, parameters are "
					"still adequate.");
		} else {
			pma new_file = {
				.keys = NULL,
				.values = NULL,
				.occupied = NULL,
			};
			pma_stream stream;
			pma_stream_start(&new_file, count, &stream);
			for (uint64_t i = 0; i < file->capacity; i++) {
				if (file->occupied[i]) {
					pma_stream_push(&stream, file->keys[i],
							file->values[i]);
				}
			}
			pma_destroy(file);
			*file = new_file;

			const pma_range whole_file = {
				.begin = 0,
				.size = file->capacity,
				.file = file,
			};
			log_verbose(1, "resized, new capacity=%" PRIu64 " "
					"occupied=%" PRIu64, file->capacity,
					pma_count_occupied(whole_file));
			range = whole_file;
			assert(pma_block_within_threshold(range));
		}
	} else {
		assert(pma_block_within_threshold(range));
	}
	PMA_COUNTERS.reorganized_size += range.size;

	pma_spread(range, watch);
	return range;
}

// Moves every item in the block, starting with index `step_start`,
// one slot to the right.
static void step_right(pma_range block, uint64_t step_start) {
	log_verbose(2, "step_right");
	// There needs to be some free space at the end.
	for (uint64_t i = block.begin + block.size - 1; i > step_start; i--) {
		assert(!block.file->occupied[i]);
		pma_move(*block.file, i, i - 1, NULL);
	}
}

pma_value pma_get_value(const pma* file, uint64_t index) {
	return file->values[index];
}

pma_range pma_insert_before(pma* file, uint64_t key, const pma_value value,
		uint64_t insert_before_index) {
	// log_verbose(2, "pma_insert_before(%" PRIu64 "=%" PRIu64 ", "
	// 		"before_index=%" PRIu64 ")", item.key, item.value,
	// 		insert_before_index);
	assert(insert_before_index <= file->capacity);

	pma_range block;
	if (insert_before_index == file->capacity) {
		// Special case. We will shift everything to the left,
		// and then insert to the end.
		block = get_leaf(file, file->capacity - 1);
	} else {
		// We will shift everything to the left,
		// then shift everything after `insert_before_index`
		// by 1 to the right and reshuffle.
		assert(file->occupied[insert_before_index]);
		block = get_leaf(file, insert_before_index);
	}

	while (pma_block_full(block) && !is_entire_file(block)) {
		block = block_parent(block);
	}

	if (pma_block_full(block) && is_entire_file(block)) {
		bool was_end = (insert_before_index == file->capacity);
		rebalance(file, (pma_range) {
			.begin = 0,
			.size = file->capacity,
			.file = file
		}, &insert_before_index);
		if (was_end) {
			insert_before_index = file->capacity;
		}
		return pma_insert_before(file, key, value, insert_before_index);
	}

	assert(!pma_block_full(block));
	pma_compact_left(block, &insert_before_index);

	if (insert_before_index == file->capacity) {
		--insert_before_index;
	} else {
		assert(insert_before_index >= block.begin &&
				insert_before_index < block.begin + block.size);
		step_right(block, insert_before_index);
	}
	assert(!file->occupied[insert_before_index]);
	file->occupied[insert_before_index] = true;
	file->keys[insert_before_index] = key;
	file->values[insert_before_index] = value;
	return rebalance(file, block, NULL);
}

pma_range pma_delete(pma* file, uint64_t index) {
	assert(file->occupied[index]);
	file->occupied[index] = false;
	return rebalance(file, get_leaf(file, index), NULL);
}

void pma_init(pma* file) {
	// TODO: copy over?
	// TODO: merge with new_ordered_file
	assert(!file->keys && !file->values && !file->occupied);
	file->capacity = 4;
	file->block_size = 4;
	file->keys = NULL;
	file->values = NULL;
	file->occupied = NULL;
	alloc_file(file);
}

void pma_destroy(pma* file) {
	free(file->keys);
	free(file->values);
	free(file->occupied);
	file->keys = NULL;
	file->values = NULL;
	file->occupied = NULL;
}

void pma_stream_start(pma* file, uint64_t size, pma_stream* stream) {
	struct parameters parameters = adequate_parameters(size);
	file->block_size = parameters.block_size;
	file->capacity = parameters.capacity;
	alloc_file(file);
	// TODO: This will be soon followed by an expensive
	// rebalancing. Perhaps we should build it rebalanced
	// in the first place.
	stream->allowed_capacity = size;
	stream->file = file;
	stream->scratch = 0;
}

void pma_stream_push(pma_stream* stream, uint64_t key, const pma_value value) {
	assert(stream->scratch < stream->allowed_capacity);
	stream->file->occupied[stream->scratch] = true;
	stream->file->keys[stream->scratch] = key;
	stream->file->values[stream->scratch] = value;
	++stream->scratch;
}
