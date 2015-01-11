#include "veb_layout.h"
#include <assert.h>
#include <inttypes.h>
#include "../math/math.h"
#include "../log/log.h"

static uint64_t m_exp2(uint64_t x) {
	return 1ULL << x;
}

static void split_height(uint64_t height, uint64_t* bottom, uint64_t* top) {
	*bottom = hyperfloor(height - 1);
	*top = height - *bottom;
}

static veb_pointer veb_pointer_add(veb_pointer base, uint64_t shift) {
	return (veb_pointer) {
		.present = base.present,
		.node = base.node + shift
	};
}

void build_veb_layout(uint64_t height,
		uint64_t node_start,
		void (*set_node)(void* opaque,
			uint64_t node, veb_pointer left, veb_pointer right),
		void* set_node_opaque,
		veb_pointer leaf_source, uint64_t leaf_stride) {
	assert(height > 0);
	if (height == 1) {
		set_node(set_node_opaque, node_start, leaf_source,
				veb_pointer_add(leaf_source, leaf_stride));
	} else {
		uint64_t bottom_height, top_height;
		split_height(height, &bottom_height, &top_height);

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t number_of_bottom_blocks = m_exp2(top_height);
		const uint64_t nodes_in_bottom_block =
			m_exp2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = m_exp2(bottom_height);

		build_veb_layout(top_height, node_start,
				set_node, set_node_opaque,
				(veb_pointer) {
					.present = true,
					.node = node_start + nodes_in_top_block
				}, nodes_in_bottom_block);
		node_start += nodes_in_top_block;

		for (uint64_t bottom_block_index = 0;
				bottom_block_index < number_of_bottom_blocks;
				bottom_block_index++) {
			build_veb_layout(bottom_height, node_start,
					set_node, set_node_opaque,
					leaf_source, leaf_stride);
			node_start += nodes_in_bottom_block;
			leaf_source = veb_pointer_add(leaf_source, leaf_stride * leaves_per_bottom_block);
		}
	}
}

void veb_get_children2(uint64_t node, uint64_t height,
		veb_pointer* left, veb_pointer* right) {
	uint64_t node_start = 0;
	veb_pointer leaf_source = (veb_pointer) {
		.present = false,
		.node = 0
	};
	uint64_t leaf_stride = 0;

	while (height > 1) {
		uint64_t bottom_height, top_height;
		split_height(height, &bottom_height, &top_height);

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t number_of_bottom_blocks = m_exp2(top_height);
		const uint64_t nodes_in_bottom_block =
			m_exp2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = m_exp2(bottom_height);

		if (node < node_start + nodes_in_top_block) {
			height = top_height;
			leaf_source.present = true;
			leaf_source.node = node_start + nodes_in_top_block;
			leaf_stride = nodes_in_bottom_block;
		} else {
			node_start += nodes_in_top_block;

			const uint64_t bottom_block_index =
					(node - node_start) / nodes_in_bottom_block;
			assert(bottom_block_index < number_of_bottom_blocks);
			height = bottom_height;
			node_start += nodes_in_bottom_block * bottom_block_index;
			leaf_source.node += leaf_stride * leaves_per_bottom_block * bottom_block_index;
		}
	}
	*left = leaf_source;
	*right = veb_pointer_add(leaf_source, leaf_stride);
}

// Conceptually derived from build_veb_layout.
// This function is easier to understand with recursion, but to get better
// speed, we change it manually to be tail-recursive.
void veb_get_children(uint64_t node, uint64_t height,
		veb_pointer* left, veb_pointer* right) {
	uint64_t node_start = 0;
	veb_pointer leaf_source = (veb_pointer) {
		.present = false,
		.node = 0
	};
	uint64_t leaf_stride = 0;

	while (height > 1) {
		uint64_t bottom_height, top_height;
		split_height(height, &bottom_height, &top_height);

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t number_of_bottom_blocks = m_exp2(top_height);
		const uint64_t nodes_in_bottom_block =
			m_exp2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = m_exp2(bottom_height);

		if (node < node_start + nodes_in_top_block) {
			height = top_height;
			leaf_source.present = true;
			leaf_source.node = node_start + nodes_in_top_block;
			leaf_stride = nodes_in_bottom_block;
		} else {
			node_start += nodes_in_top_block;

			const uint64_t bottom_block_index =
					(node - node_start) / nodes_in_bottom_block;
			assert(bottom_block_index < number_of_bottom_blocks);
			height = bottom_height;
			node_start += nodes_in_bottom_block * bottom_block_index;
			leaf_source.node += leaf_stride * leaves_per_bottom_block * bottom_block_index;
		}
	}
	*left = leaf_source;
	*right = veb_pointer_add(leaf_source, leaf_stride);
}

bool veb_is_leaf(uint64_t node, uint64_t height) {
	uint64_t node_start = 0;
recursive_call:
	assert(height > 0);
	if (height == 1) {
		return true;
	} else {
		uint64_t bottom_height, top_height;
		split_height(height, &bottom_height, &top_height);

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t number_of_bottom_blocks = m_exp2(top_height);
		const uint64_t nodes_in_bottom_block =
			m_exp2(bottom_height) - 1;

		if (node < node_start + nodes_in_top_block) {
			height = top_height;
			return false;
		}
		node_start += nodes_in_top_block;

		// TODO: Optimize away this loop.
		const uint64_t bottom_block_index = (node - node_start) / nodes_in_bottom_block;
		assert(bottom_block_index < number_of_bottom_blocks);
		node_start += nodes_in_bottom_block * bottom_block_index;
		height = bottom_height;
		goto recursive_call;
	}
}

static uint64_t must_have(veb_pointer ptr) {
	assert(ptr.present);
	return ptr.node;
}

uint64_t veb_get_leaf_number(uint64_t leaf_index, uint64_t height) {
	// TODO: probably can be optimized
	uint64_t power = m_exp2(height - 2);
	uint64_t x = leaf_index;
	uint64_t ptr = 0;  // root
	while (!veb_is_leaf(ptr, height)) {
		veb_pointer left, right;
		veb_get_children(ptr, height, &left, &right);
		if (x / power == 0) {
			ptr = must_have(left);
		} else {
			ptr = must_have(right);
		}
		x %= power;
		power /= 2;
	}
	return ptr;
}

// Conceptually derived from build_veb_layout
uint64_t veb_get_leaf_index_of_leaf(uint64_t node, uint64_t height) {
	uint64_t offset = 0;
recursive_call:
	assert(height > 0);
	if (height == 1) {
		assert(node == 0);
		return offset;
	} else {
		uint64_t bottom_height, top_height;
		split_height(height, &bottom_height, &top_height);

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t nodes_in_bottom_block =
			m_exp2(bottom_height) - 1;

		// If we ever go up, it's not a leaf anymore.
		assert(node >= nodes_in_top_block);

		const uint64_t bottom_block_index = (node - nodes_in_top_block) / nodes_in_bottom_block;
		const uint64_t bottom_block_node = (node - nodes_in_top_block) % nodes_in_bottom_block;

		offset += bottom_block_index * m_exp2(bottom_height - 1);
		node = bottom_block_node;
		height = bottom_height;
		goto recursive_call;
	}
}
