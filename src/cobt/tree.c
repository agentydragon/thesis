#include "cobt/tree.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#define NO_LOG_INFO
#include "log/log.h"
#include "math/math.h"

static const uint64_t INFINITY = UINT64_MAX;

static uint8_t height(cobt_tree* this) {
	return ceil_log2(this->backing_array_size) + 1;
}

static uint64_t tree_node_count(cobt_tree* this) {
	return (1 << height(this)) - 1;
}

static cobt_tree_range entire_tree(cobt_tree* this) {
	return (cobt_tree_range) {
		.begin = 0,
		.end = 1 << (height(this) - 1)
	};
}

void cobt_tree_init(cobt_tree* this, const uint64_t* backing_array,
		const bool* backing_array_occupied,
		uint64_t backing_array_size) {
	this->backing_array = backing_array;
	this->backing_array_occupied = backing_array_occupied;
	this->backing_array_size = backing_array_size;

	this->tree = calloc(tree_node_count(this), sizeof(uint64_t));
	assert(this->tree);

	this->level_data = calloc(height(this), sizeof(struct level_data));
	assert(this->level_data);

	veb_prepare(height(this), this->level_data);
	cobt_tree_refresh(this, entire_tree(this));
}

void cobt_tree_destroy(cobt_tree* this) {
	free(this->tree);
	this->tree = NULL;

	free(this->level_data);
	this->level_data = NULL;
}

uint64_t cobt_tree_find_le(cobt_tree* this, uint64_t key) {
	//log_info("finding max index <= key %" PRIu64, key);
	uint8_t stack_size = 0;

	struct drilldown_track track;
	drilldown_begin(&track);

	uint64_t leaf_index = 0;
	do {
		stack_size++;

		if (stack_size == height(this)) {
			//log_info(" => %" PRIu64, leaf_index);
			return leaf_index;
		} else {
			// NOTE: this is the reason why right drilldowns
			// are more likely.
			drilldown_go_right(this->level_data, &track);
			if (key >= this->tree[track.pos[track.depth]]) {
				// We want to go right.
				leaf_index = (leaf_index << 1) + 1;
			} else {
				drilldown_go_up(&track);
				drilldown_go_left(this->level_data, &track);
				leaf_index = leaf_index << 1;
			}
		}
	} while (true);

	return leaf_index;
}

static uint64_t size(cobt_tree_range range) {
	return range.end - range.begin;
}

static uint64_t midpoint(cobt_tree_range range) {
	return range.begin + size(range) / 2;
}

static cobt_tree_range left_half(cobt_tree_range range) {
	return (cobt_tree_range) {
		.begin = range.begin,
		.end = midpoint(range)
	};
}

static cobt_tree_range right_half(cobt_tree_range range) {
	return (cobt_tree_range) {
		.begin = midpoint(range),
		.end = range.end
	};
}

static bool is_subrange_of(cobt_tree_range subrange, cobt_tree_range range) {
	return (subrange.begin >= range.begin && subrange.end <= range.end);
}

static bool disjoint(cobt_tree_range a, cobt_tree_range b) {
	return (a.begin >= b.end) || (b.begin >= a.end);
}

static bool intersect(cobt_tree_range a, cobt_tree_range b) {
	return !disjoint(a, b);
}

static void refresh_recursive(cobt_tree* this, cobt_tree_range refresh,
		cobt_tree_range current, struct drilldown_track* track) {
	const uint64_t current_nid = track->pos[track->depth];
	log_info("refresh=[%" PRIu64 ",%" PRIu64 ") "
			"current=[%" PRIu64 ",%" PRIu64 ") nid=%" PRIu64,
			refresh.begin, refresh.end,
			current.begin, current.end,
			current_nid);
	const uint64_t old = this->tree[current_nid];

	if (size(current) == 1) {
		if (current.begin < this->backing_array_size &&
				this->backing_array_occupied[current.begin]) {
			this->tree[current_nid] = this->backing_array[current.begin];
		} else {
			this->tree[current_nid] = INFINITY;
		}
	} else {
		this->tree[current_nid] = UINT64_MAX; // XX: NEEDED

		drilldown_go_left(this->level_data, track);
		if (intersect(refresh, left_half(current))) {
			refresh_recursive(this, refresh,
					left_half(current), track);
		}
		const uint64_t left = this->tree[track->pos[track->depth]];
		drilldown_go_up(track);
		drilldown_go_right(this->level_data, track);
		if (intersect(refresh, right_half(current))) {
			refresh_recursive(this, refresh,
					right_half(current), track);
		}
		const uint64_t right = this->tree[track->pos[track->depth]];
		drilldown_go_up(track);
		if (left < right) {
			this->tree[current_nid] = left;
		} else {
			this->tree[current_nid] = right;
		}
	}
	if (this->tree[current_nid] == INFINITY) {
		if (old != this->tree[current_nid]) {
			log_info("=> %" PRIu64 " fixed to INFINITY", current_nid);
		} else {
			log_info("=> %" PRIu64 " remains INFINITY", current_nid);
		}
	} else {
		if (old != this->tree[current_nid]) {
			log_info("=> %" PRIu64 " fixed to %" PRIu64,
					current_nid, this->tree[current_nid]);
		} else {
			log_info("=> %" PRIu64 " remains at %" PRIu64,
					current_nid, this->tree[current_nid]);
		}
	}
}

void cobt_tree_refresh(cobt_tree* this, cobt_tree_range refresh) {
	struct drilldown_track track;
	drilldown_begin(&track);
	refresh_recursive(this, refresh, entire_tree(this), &track);
}

void cobt_tree_dump(cobt_tree* this) {
	char buffer[4096];
	uint64_t z = 0;
	for (uint64_t i = 0; i < tree_node_count(this); i++) {
		if (this->tree[i] != INFINITY) {
			z += sprintf(buffer + z, "[%" PRIu64 "]=%" PRIu64 " ",
					i, this->tree[i]);
		} else {
			z += sprintf(buffer + z, "[%" PRIu64 "]=INFINITY ", i);
		}
	}
	log_info("%s", buffer);
}
