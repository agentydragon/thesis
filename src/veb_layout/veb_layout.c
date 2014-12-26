#include "veb_layout.h"
#include <assert.h>
#include "../math/math.h"
#define NO_LOG_INFO
#include "../log/log.h"

static uint64_t m_exp2(uint64_t x) {
	uint64_t log, n;
	for (log = 0, n = 1; log < x; n *= 2, ++log);
	return n;
}

static void split_height(uint64_t height, uint64_t* bottom, uint64_t* top) {
	if (closest_pow2_floor(height) != height) {
		*bottom = closest_pow2_floor(height - 1);
		*top = height - *bottom;
	} else {
		*bottom = height / 2;
		*top = height / 2;
	}
	assert(*top + *bottom == height);
}

void build_veb_layout(uint64_t height, veb_node* space,
		veb_node* leaf_start, uint64_t leaf_stride) {
	log_info("build_veb_layout(height=%d,space=%p,leaf_start=%p,leaf_stride=%d)",
			height, space, leaf_start, leaf_stride);
	if (height == 0) {
		return;
	} else if (height == 1) {
		space[0].left = leaf_start;
		space[0].right = leaf_start + leaf_stride;
	} else {
		uint64_t bottom_height, top_height;
		split_height(height, &bottom_height, &top_height);

		log_info("bottom_height=%d top_height=%d",
				bottom_height, top_height);

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t number_of_bottom_blocks = m_exp2(top_height);
		const uint64_t nodes_in_bottom_block =
			m_exp2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = m_exp2(bottom_height);

		veb_node* my_leaf_start = space + nodes_in_top_block;

		veb_node* write_to = space;
		build_veb_layout(top_height, write_to,
				my_leaf_start, nodes_in_bottom_block);
		write_to += nodes_in_top_block;

		for (uint64_t bottom_block_index = 0;
				bottom_block_index < number_of_bottom_blocks;
				bottom_block_index++) {
			log_info("-> building leaf %d/%d",
					bottom_block_index + 1, bottom_blocks);
			build_veb_layout(bottom_height, write_to,
					leaf_start + (bottom_block_index * leaf_stride * leaves_per_bottom_block),
					leaf_stride);
			write_to += nodes_in_bottom_block;
		}
	}
}
