#ifndef VEB_LAYOUT_H
#define VEB_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

typedef uint64_t veb_pointer;

#define VP_NO_NODE 0xFFFFFFFFFFFFFFFFULL
#define VP_PRESENT(x) ((x) != VP_NO_NODE)
#define VP_ADD(x,offset) (VP_PRESENT(x) ? ((x) + (offset)) : VP_NO_NODE)

// Alternate bit-fiddling:
// #define VP_NO_NODE 0x8000000000000000ULL
// #define VP_PRESENT(x) (!((x) & VP_NO_NODE))

typedef struct veb_children {
	veb_pointer left, right;
} veb_children;

// Primary API
veb_children veb_get_children(uint64_t node, uint64_t height);

// Used only internally to build cache.
// node: leaf node index in van Emde Boas order
// returns: number of the leaf (index from left to right amongst leaves)
uint64_t veb_get_leaf_index_of_leaf(uint64_t node, uint64_t height);

// Used only in testing.
void build_veb_layout(uint64_t height,
		uint64_t node_start,
		void (*set_node)(void* opaque,
			uint64_t node, veb_pointer left, veb_pointer right),
		void* set_node_opaque,
		veb_pointer leaf_source, uint64_t leaf_stride);
bool veb_is_leaf(uint64_t node, uint64_t height);
uint64_t veb_get_leaf_number(uint64_t leaf_index, uint64_t height);

#endif
