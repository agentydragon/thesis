#ifndef VEB_LAYOUT_H
#define VEB_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

// PREFERRED API:
struct level_data {
	uint64_t top_size;
	uint64_t bottom_size;
	uint64_t top_depth;
};
struct level_data veb_get_level_data(uint64_t height, uint64_t level);
void veb_prepare(uint64_t height, struct level_data* levels);

struct drilldown_track {
	uint64_t pos[50];  /* TODO: maybe dynamic alloc? */
	uint8_t depth;     /* pos[depth] = veb position */
	uint64_t bfs;
};
void drilldown_begin(struct drilldown_track* track);
void drilldown_go_left(struct level_data* ld, struct drilldown_track* track);
void drilldown_go_right(struct level_data* ld, struct drilldown_track* track);
void drilldown_go_up(struct drilldown_track* track);

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
