#ifndef VEB_LAYOUT_H
#define VEB_LAYOUT_H

#include <stdint.h>

struct veb_node;
typedef struct veb_node veb_node;

struct veb_node {
	veb_node *left, *right;
};

void build_veb_layout(uint64_t height, veb_node* space,
		veb_node* leaf_start, uint64_t leaf_stride);

#endif
