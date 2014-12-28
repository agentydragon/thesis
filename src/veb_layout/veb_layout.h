#ifndef VEB_LAYOUT_H
#define VEB_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct veb_pointer {
	bool present;
	uint64_t node;
} veb_pointer;

void build_veb_layout(uint64_t height,
		uint64_t node_start,
		void (*set_node)(void* opaque,
			uint64_t node, veb_pointer left, veb_pointer right),
		void* set_node_opaque,
		veb_pointer leaf_source, uint64_t leaf_stride);

bool veb_is_leaf(uint64_t node, uint64_t height);
veb_pointer veb_get_left(uint64_t node, uint64_t height);
veb_pointer veb_get_right(uint64_t node, uint64_t height);

// TODO: maybe this one is useless?
uint64_t veb_get_leaf_number(uint64_t leaf_index, uint64_t height);

// node: leaf node index in van Emde Boas order
// returns: number of the leaf (index from left to right amongst leaves)
uint64_t veb_get_leaf_index_of_leaf(uint64_t node, uint64_t height);

#endif
