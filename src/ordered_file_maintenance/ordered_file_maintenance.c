#include "ordered_file_maintenance.h"
#include "../math/math.h"

#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NO_LOG_INFO
#include "../log/log.h"

#define INVALID_INDEX 0xDEADBEEFDEADBEEF
const struct watched_index NO_WATCH = {
	.index = INVALID_INDEX,
	.new_location = NULL
};

// TODO: Even spread by mohl fungovat tak, ze by si Reorganize postupne
// budoval kompaktni blok kolem "zacatku" zatimco by pocital pocet obsazenych
// prvku. Potom bych mohl provest even spread se znamym mnozstvim prvku
// (neco jako funguje ted, akorat modulo).

struct parameters adequate_parameters(uint64_t items) {
	assert(items < INVALID_INDEX);
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

		//log_info("capacity=%" PRIu64 " size=%" PRIu64 " "
		//		"gives density=%lf", capacity, items,
		//		density);

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

void range_describe(struct ordered_file file,
		struct ordered_file_range range, char* buffer) {
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

static uint64_t get_leaf_depth(struct parameters parameters) {
	if (parameters.capacity == parameters.block_size) {
		// XXX: hack for special small case
		return 1;
	}
	return ceil_log2(parameters.capacity / parameters.block_size);
}

static void copy_item(ordered_file_item* to, ordered_file_item from) {
	// TODO: or maybe two assignments? this should probably be optimizable.
	memcpy(to, &from, sizeof(ordered_file_item));
}

static void move_item(struct ordered_file file, uint64_t to, uint64_t from,
		struct watched_index* watched_index) {
	file.occupied[from] = false;
	file.occupied[to] = true;
	copy_item(&file.items[to], file.items[from]);
	if (from == watched_index->index) {
		log_info("move_item: watched: %" PRIu64 " ==> %" PRIu64, from, to);
		*(watched_index->new_location) = to;
		watched_index->index = to;
	}
}

// Shifts all elements of the range to the left.
void range_compact(
		struct ordered_file file, struct ordered_file_range range,
		struct watched_index watched_index) {
	for (uint64_t source = 0, target = 0; source < range.size; source++) {
		const uint64_t from = range.begin + source;
		if (file.occupied[from]) {
			file.occupied[from] = false;
			while (file.occupied[range.begin + target]) target++;
			const uint64_t to = range.begin + target;
			file.occupied[to] = true;
			copy_item(&file.items[to], file.items[from]);
			if (from == watched_index.index) {
				log_info("compact moves %" PRIu64 " to %" PRIu64,
						watched_index.index, to);
				*(watched_index.new_location) = to;
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

static uint64_t file_get_occupied(struct ordered_file file) {
	return range_get_occupied(file, (struct ordered_file_range) {
		.begin = 0,
		.size = file.parameters.capacity
	});
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
		struct ordered_file_range range,
		ordered_file_item inserted_item) {
	assert(!range_is_full(file, range));
	// TODO: optimize
	range_compact(file, range, NO_WATCH);
	const uint64_t occupied = range_get_occupied(file, range);
	for (uint64_t i = 0; i < occupied; i++) {
		const uint64_t to = range.begin + occupied - i,
				from = range.begin + occupied - i - 1;
		file.occupied[to] = file.occupied[from];
		copy_item(&file.items[to], file.items[from]);
	}
	file.occupied[range.begin] = true;
	copy_item(&file.items[range.begin], inserted_item);
}

void range_insert_after(struct ordered_file file,
		struct ordered_file_range range, ordered_file_item inserted_item,
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
		copy_item(&file.items[to], file.items[from]);

		if (from == insert_after_index) {
			break;
		}

		CHECK(copy_from != 0, "we are trying to copy to wrong places");
	}
	copy_item(&file.items[insert_after_index + 1], inserted_item);
}

static void evenly_spread_offline(struct ordered_file source_file,
		struct ordered_file target_file,
		struct watched_index watched_index) {
	const uint64_t elements = file_get_occupied(source_file);
	// TODO: avoid float arithmetic
	// TOOD: maybe merge with range_spread_evenly?
	const double gap_size = ((double) target_file.parameters.capacity) /
			elements;
	assert(gap_size >= 1.0);
	uint64_t element_index = 0;
	for (uint64_t i = 0; i < source_file.parameters.capacity; i++) {
		if (source_file.occupied[i]) {
			const uint64_t target_index = round(
					element_index * gap_size);
			target_file.occupied[target_index] = true;
			copy_item(&target_file.items[target_index],
					source_file.items[i]);
			element_index++;
			if (i == watched_index.index) {
				*(watched_index.new_location) = target_index;
			}
		}
	}
}

void range_spread_evenly(struct ordered_file file,
		struct ordered_file_range range,
		struct watched_index watched_index) {
	uint64_t sublocation = INVALID_INDEX;
	range_compact(file, range, (struct watched_index) {
		.index = watched_index.index,
		.new_location = &sublocation
	});
	const uint64_t elements = range_get_occupied(file, range);

	// TODO: avoid float arithmetic
	const double gap_size = ((double) range.size) / elements;
	for (uint64_t i = 0; i < elements; i++) {
		const uint64_t target_index =
				range.size - 1 - round(i * gap_size);

		const uint64_t from = range.begin + elements - 1 - i,
				to = range.begin + target_index;
		copy_item(&file.items[to], file.items[from]);
		file.occupied[from] = false;
		file.occupied[to] = true;
		if (sublocation == from) {
			log_info("even spread moves %" PRIu64 " to %" PRIu64,
					sublocation, to);
			*(watched_index.new_location) = to;
		}
	}
}

bool dense_enough(uint64_t size, uint64_t capacity,
		uint64_t depth, uint64_t structure_height) {
	// used >= available/2 - available/4 depth/s_height
	// 4*used >= 2*available - available depth/s_height
	// 4*used*s_height >= 2*available*s_height - available*depth
	// used / available >= 1/2 - (1/4) (depth / s_height)
	return (4 * size * structure_height >=
			2 * capacity * structure_height - capacity * depth);
}

bool sparse_enough(uint64_t size, uint64_t capacity,
		uint64_t depth, uint64_t structure_height) {
	// used / available <= 3/4 + (1/4) (depth / s_height)
	// 4*used*s_height <= 3*available*s_height + available*depth
	return (4 * size * structure_height <=
			3 * capacity * structure_height - capacity * depth);
}

bool global_density_within_threshold(uint64_t size, uint64_t capacity) {
	// Workaround for really small sets.
	if (size <= 4) {
		return capacity > size;
	}
	return density_is_within_threshold(size, capacity, 0, 1);
}

bool density_is_within_threshold(uint64_t size, uint64_t capacity,
		uint64_t depth, uint64_t structure_height) {
	return dense_enough(size, capacity, depth, structure_height) &&
		sparse_enough(size, capacity, depth, structure_height);
}

static struct ordered_file_range get_leaf_range_for(struct ordered_file file,
		uint64_t index) {
	return get_leaf_range(file, index / file.parameters.block_size);
}

static void destroy_ordered_file(struct ordered_file file) {
	free(file.occupied);
	free(file.items);
}

static struct ordered_file new_ordered_file(struct parameters parameters) {
	struct ordered_file file = {
		.occupied = calloc(parameters.capacity, sizeof(bool)),
		.items = calloc(parameters.capacity, sizeof(ordered_file_item)),
		.parameters = parameters
	};
	// TODO: NULL-resistance
	assert(file.occupied && file.items);
	return file;
}

// Used for debugging.
static void __attribute__((unused)) dump_file(struct ordered_file file) {
	log_info("capacity=%" PRIu64 " block_size=%" PRIu64,
			file.parameters.capacity,
			file.parameters.block_size);
	char buffer[256];
	uint64_t offset = 0;
	for (uint64_t i = 0; i < file.parameters.capacity; i++) {
		if (i % file.parameters.block_size == 0 && i != 0) {
			offset += sprintf(buffer + offset, "\n");
		}
		if (file.occupied[i]) {
			offset += sprintf(buffer + offset, "%3" PRIu64 " ",
					file.items[i].key);
		} else {
			offset += sprintf(buffer + offset, "--- ");
		}
	}
	log_info("items={%s}", buffer);
}

static void range_spread_evenly2(struct ordered_file file,
		struct ordered_file_range range, uint64_t left, uint64_t right,
		struct watched_index *watched_index) {
	const uint64_t occupied = right - left;
	const double gap_size = ((double) range.size) / occupied;

	uint64_t left_index = left, right_index = right;

	log_info("=== range_spread_evenly2 size=%" PRIu64 " ==", range.size);
	while (left_index != right_index) {
		const uint64_t left_move_at =
			range.begin + round((left_index - left) * gap_size);
		if (left_move_at > left_index) {
			break;
		}
		log_info("left move: %" PRIu64 " <-- %" PRIu64,
				left_move_at, left_index);
		move_item(file, left_move_at, left_index, watched_index);
		left_index++;
	}

	while (left_index != right_index) {
		const uint64_t right_move_at =
			range.begin +
				round((right_index - left - 1) * gap_size);
		if (right_move_at < right_index - 1) {
			break;
		}
		log_info("right move: %" PRIu64 " --> %" PRIu64,
				right_index - 1, right_move_at);
		move_item(file, right_move_at, right_index - 1, watched_index);
		right_index--;
	}

	assert(left_index == right_index);
}

static struct ordered_file_range reorganize(struct ordered_file* file,
		uint64_t index, struct watched_index watched_index) {
	const uint64_t leaf_depth = get_leaf_depth(file->parameters);
	uint64_t depth = leaf_depth;

	struct ordered_file_range block = get_leaf_range_for(*file, index);

	// left_pointer = first occupied
	// right_pointer = after last occupied
	uint64_t left_pointer = block.begin;
	uint64_t right_pointer = block.begin;

	for (uint64_t i = 0; i < block.size; i++) {
		const uint64_t index = block.begin + i;
		if (file->occupied[index]) {
			log_info("initial compression: %" PRIu64 " => %" PRIu64,
					index, right_pointer);
			move_item(*file, right_pointer, index, &watched_index);
			right_pointer++;
		}
	}

	while (true) {
		const uint64_t occupied = right_pointer - left_pointer;
		log_info("level %" PRIu64 ": size=%" PRIu64 " occupied=%" PRIu64,
				depth, block.size, occupied);
		log_info("left=%" PRIu64 " right=%" PRIu64, left_pointer, right_pointer);

		if (density_is_within_threshold(
				occupied, block.size, depth, leaf_depth)) {
			log_info("within threshold, spreading");
			range_spread_evenly2(*file, block,
					left_pointer, right_pointer,
					&watched_index);
			return block;
		}

		if (block.size == file->parameters.capacity) {
			break;
		}

		if (block.begin % (block.size * 2) == 0) {
			// Extend to the right.
			for (uint64_t i = block.size; i < block.size * 2; i++) {
				const uint64_t index = block.begin + i;
				if (file->occupied[index]) {
					move_item(*file, right_pointer, index,
							&watched_index);
					right_pointer++;
				}
			}
		} else {
			// Extend to the left.
			for (uint64_t i = 0; i < block.size; i++) {
				const uint64_t index = block.begin - 1 - i;
				if (file->occupied[index]) {
					left_pointer--;
					move_item(*file, left_pointer, index,
							&watched_index);
				}
			}
			block.begin -= block.begin % (block.size * 2);
		}
		block.size *= 2;
		depth--;
	}

	{
		const uint64_t occupied = right_pointer - left_pointer;
		log_info("Changing ordered file parameters");
		struct parameters new_parameters = adequate_parameters(occupied);
		// TODO: do nothing if new_parameters == this->parameters.
		struct ordered_file resized = new_ordered_file(new_parameters);
		evenly_spread_offline(*file, resized, watched_index);
		destroy_ordered_file(*file);
		*file = resized;
	}

	// Everything was reorged.
	return (struct ordered_file_range) {
		.begin = 0,
		.size = file->parameters.capacity
	};
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
		ordered_file_item item) {
	struct ordered_file_range reorg_range = reorganize(file, 0, NO_WATCH);
	struct ordered_file_range leaf = get_leaf_range_for(*file, 0);
	CHECK(!range_is_full(*file, leaf), "reorganization didn't help");
	range_insert_first(*file, leaf, item);
	return reorg_range;
}

struct ordered_file_range ordered_file_insert_after(struct ordered_file* file,
		ordered_file_item item, uint64_t insert_after_index) {
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
	CHECK(index < file->parameters.capacity, "Deleting out of range");
	CHECK(file->occupied[index], "Deleting empty index %" PRIu64, index);
	file->occupied[index] = false;
	return reorganize(file, index, NO_WATCH);
}

void ordered_file_init(struct ordered_file* file) {
	// TODO: copy over?
	// TODO: merge with new_ordered_file
	file->parameters.capacity = 4;
	file->parameters.block_size = 4;
	file->items = calloc(4, sizeof(ordered_file_item));
	file->occupied = calloc(4, sizeof(bool));

	for (uint8_t i = 0; i < 4; i++) {
		file->occupied[i] = false;
	}

	assert(file->items && file->occupied);
}

void ordered_file_destroy(struct ordered_file file) {
	free(file.items);
	free(file.occupied);
}
