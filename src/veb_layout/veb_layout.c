#include "veb_layout.h"
#include <assert.h>
#include <inttypes.h>
#include "../math/math.h"
//#define NO_LOG_INFO
#include "../log/log.h"

static uint64_t m_exp2(uint64_t x) {
	return 1ULL << x;
}

static void split_height(uint64_t height, uint64_t* bottom, uint64_t* top) {
	*bottom = closest_pow2_floor(height - 1);
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
			//log_info("-> building leaf %d/%d",
			//		bottom_block_index + 1, bottom_blocks);
			build_veb_layout(bottom_height, node_start,
					set_node, set_node_opaque,
					leaf_source, leaf_stride);
			node_start += nodes_in_bottom_block;
			leaf_source = veb_pointer_add(leaf_source, leaf_stride * leaves_per_bottom_block);
		}
	}
}

// Conceptually derived from build_veb_layout.
// TODO: change tail-recursion to cycle
veb_pointer veb_get_left(uint64_t node,
		uint64_t height,
		uint64_t node_start,
		veb_pointer leaf_source, uint64_t leaf_stride) {
	assert(height > 0);
	if (height == 1) {
		return leaf_source;
	} else {
		uint64_t bottom_height, top_height;
		split_height(height, &bottom_height, &top_height);

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t number_of_bottom_blocks = m_exp2(top_height);
		const uint64_t nodes_in_bottom_block =
			m_exp2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = m_exp2(bottom_height);

		if (node < node_start + nodes_in_top_block) {
			return veb_get_left(node,
					top_height, node_start,
					(veb_pointer) {
						.present = true,
						.node = node_start + nodes_in_top_block
					}, nodes_in_bottom_block);
		}
		node_start += nodes_in_top_block;

		for (uint64_t bottom_block_index = 0;
				bottom_block_index < number_of_bottom_blocks;
				bottom_block_index++) {
			if (node < node_start + nodes_in_bottom_block) {
				return veb_get_left(node,
						bottom_height, node_start,
						leaf_source, leaf_stride);
			}
			node_start += nodes_in_bottom_block;
			leaf_source = veb_pointer_add(leaf_source, leaf_stride * leaves_per_bottom_block);
		}
		log_fatal("fail");
	}
}

// Conceptually derived from build_veb_layout.
// TODO: change tail-recursion to cycle
veb_pointer veb_get_right(uint64_t node,
		uint64_t height,
		uint64_t node_start,
		veb_pointer leaf_source, uint64_t leaf_stride) {
	assert(height > 0);
	if (height == 1) {
		return veb_pointer_add(leaf_source, leaf_stride);
	} else {
		uint64_t bottom_height, top_height;
		split_height(height, &bottom_height, &top_height);

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t number_of_bottom_blocks = m_exp2(top_height);
		const uint64_t nodes_in_bottom_block =
			m_exp2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = m_exp2(bottom_height);

		if (node < node_start + nodes_in_top_block) {
			return veb_get_right(node,
					top_height, node_start,
					(veb_pointer) {
						.present = true,
						.node = node_start + nodes_in_top_block
					}, nodes_in_bottom_block);
		}
		node_start += nodes_in_top_block;

		for (uint64_t bottom_block_index = 0;
				bottom_block_index < number_of_bottom_blocks;
				bottom_block_index++) {
			if (node < node_start + nodes_in_bottom_block) {
				return veb_get_right(node,
						bottom_height, node_start,
						leaf_source, leaf_stride);
			}
			node_start += nodes_in_bottom_block;
			leaf_source = veb_pointer_add(leaf_source, leaf_stride * leaves_per_bottom_block);
		}
		log_fatal("fail");
	}
}
