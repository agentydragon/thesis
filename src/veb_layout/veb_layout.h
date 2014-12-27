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
		void (*set_node)(uint64_t node, veb_pointer left, veb_pointer right),
		veb_pointer leaf_source, uint64_t leaf_stride);

#endif
