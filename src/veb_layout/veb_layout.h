#ifndef VEB_LAYOUT_H
#define VEB_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

// PREFERRED API:
typedef struct {
	uint64_t top_size;
	uint64_t bottom_size;
	uint64_t top_depth;
} veb_level_data;
veb_level_data veb_get_level_data(uint64_t height, uint64_t level);
void veb_prepare(uint64_t height, veb_level_data* levels);

struct drilldown_track {
	uint64_t pos[50];  /* TODO: maybe dynamic alloc? */
	uint8_t depth;     /* pos[depth] = veb position */
	uint64_t bfs;
};

inline void drilldown_begin(struct drilldown_track* track) {
	track->pos[0] = 0;
	track->depth = 0;
	track->bfs = 0;
}

// add_level, drilldown_go_left, drilldown_go_right inlined for speed
inline void add_level(const veb_level_data* level_data,
		struct drilldown_track* track) {
	++track->depth;
	track->pos[track->depth] =
		track->pos[level_data[track->depth].top_depth] +
		level_data[track->depth].top_size +
		((track->bfs + 1) & level_data[track->depth].top_size) *
				level_data[track->depth].bottom_size;
}

inline void drilldown_go_left(const veb_level_data* level_data,
		struct drilldown_track* track) {
	track->bfs = (track->bfs << 1ULL) + 1;
	add_level(level_data, track);
}

inline void drilldown_go_right(const veb_level_data* level_data,
		struct drilldown_track* track) {
	track->bfs = (track->bfs << 1ULL) + 2;
	add_level(level_data, track);
}

// Navigates from a right child to its left sibling, but faster than
// go_up + go_right.
inline void drilldown_go_left_sibling(const veb_level_data* level_data,
		struct drilldown_track* track) {
	track->bfs--;
	// Removed for performance:
	//     assert(track->bfs & level_data[track->depth].top_size);
	track->pos[track->depth] -= level_data[track->depth].bottom_size;
}

// Inverse of drilldown_go_(left|right)
inline void drilldown_go_up(struct drilldown_track* track) {
	track->depth--;
	if (track->bfs % 2) {
		track->bfs = (track->bfs - 1) >> 1ULL;
	} else {
		track->bfs = (track->bfs - 2) >> 1ULL;
	}
}

// OLD API:
typedef struct veb_pointer {
	bool present;
	uint64_t node;
} veb_pointer;
// Used only in testing.
void build_veb_layout(uint64_t height,
		uint64_t node_start,
		void (*set_node)(void* opaque,
			uint64_t node, veb_pointer left, veb_pointer right),
		void* set_node_opaque,
		veb_pointer leaf_source, uint64_t leaf_stride);
bool veb_is_leaf(uint64_t node, uint64_t height);
uint64_t veb_get_leaf_number(uint64_t leaf_index, uint64_t height);

// OLD API:
void veb_get_children(uint64_t node, uint64_t height,
		veb_pointer* left, veb_pointer* right);
// Used only internally to build cache.
// node: leaf node index in van Emde Boas order
// returns: number of the leaf (index from left to right amongst leaves)
uint64_t veb_get_leaf_index_of_leaf(uint64_t node, uint64_t height);

#endif
