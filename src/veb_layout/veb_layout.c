#include "veb_layout.h"
#include <assert.h>
#include <inttypes.h>
#include "../math/math.h"
#include "../log/log.h"

static uint64_t m_exp2(uint64_t x) {
	return 1ULL << x;
}

static void split_height(uint8_t height, uint8_t* bottom, uint8_t* top) {
	*bottom = hyperfloor8(height - 1);
	*top = height - *bottom;
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
				VP_ADD(leaf_source, leaf_stride));
	} else {
		uint8_t bottom_height, top_height;
		split_height(height, &bottom_height, &top_height);

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t number_of_bottom_blocks = m_exp2(top_height);
		const uint64_t nodes_in_bottom_block =
			m_exp2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = m_exp2(bottom_height);

		build_veb_layout(top_height, node_start,
				set_node, set_node_opaque,
				node_start + nodes_in_top_block,
				nodes_in_bottom_block);
		node_start += nodes_in_top_block;

		for (uint64_t bottom_block_index = 0;
				bottom_block_index < number_of_bottom_blocks;
				bottom_block_index++) {
			build_veb_layout(bottom_height, node_start,
					set_node, set_node_opaque,
					leaf_source, leaf_stride);
			node_start += nodes_in_bottom_block;
			leaf_source = VP_ADD(leaf_source, leaf_stride * leaves_per_bottom_block);
		}
	}
}

/* possible change:
#define CACHED_TYPE uint16_t
#define CACHE_SIZE 65536
#define CACHE_PTR_TYPE uint16_t
*/

#define CACHED_TYPE uint8_t
#define CACHE_SIZE 512
#define CACHE_PTR_TYPE uint8_t

#define CACHED_TYPE_BITS (8 * sizeof(CACHED_TYPE))
#define CACHE_CHILD_BIT (1 << (CACHED_TYPE_BITS - 1))
#define C(x) ((x) | CACHE_CHILD_BIT)
#define ISC(x) ((x) & CACHE_CHILD_BIT)
#define UNC(x) ((x) & (~CACHE_CHILD_BIT))

#define MAX_CACHED_NODE_INDEX CACHE_CHILD_BIT

// TODO: wrap in struct
static CACHE_PTR_TYPE max_cached_height = 0;
static CACHE_PTR_TYPE cache_starts[256];
static CACHED_TYPE cache[CACHE_SIZE];
static bool should_build_cache = true;

static inline uint64_t fls(uint64_t f) {
	uint64_t order;
	for (order = 0; f; f >>= 1, order++) ;
	return order;
}

uint64_t ilog2(uint64_t f) {
	return fls(f) - 1;
}

static inline uint64_t hyperceil(uint64_t f) {
	return 1 << fls(f-1);
}

uint64_t bfs_to_veb_recur(uint64_t bfs_number, uint8_t height) {
//	int top_height, bottom_height;
//	int depth;
//	int subtree_depth, subtree_root, num_subtrees;
//	int toptree_size, subtree_size;
	uint8_t top_height, bottom_height;
	uint8_t depth;
	uint8_t subtree_depth;
	uint64_t subtree_root, num_subtrees;
	uint64_t toptree_size, subtree_size;
	uint64_t offset = 0;

	bfs_number++;  // (stupidity adjustment)

recurse:
	//log_info("bfs_to_veb_recur(%" PRIu64 ",%" PRIu8 ")", bfs_number, height);
	/* if this is a size-3 tree, bfs number is sufficient */
	if (height <= 2) {
		bfs_number--;  // (stupidity adjustment)
		return offset + bfs_number;
	}
	/* depth is level of the specific node */
	depth = ilog2(bfs_number);
	/* the vEB layout recursively splits the tree in half */
	bottom_height = hyperceil((height + 1) / 2);
	top_height = height - bottom_height;
	/* node is located in top half - recurse */
	if (depth < top_height) {
		// bfs_number--;  // (stupidity adjustment)
		// return bfs_to_veb_recur(bfs_number, top_height);
		height = top_height;
		goto recurse;
	}
	/*
	 * Each level adds another bit to the BFS number in the least
	 * significant position. Thus we can find the subtree root by
	 * shifting off depth - top_height rightmost bits.
	 */
	subtree_depth = depth - top_height;
	subtree_root = bfs_number >> subtree_depth;
	/*
	 * Similarly, the new bfs_number relative to subtree root has
	 * the bit pattern representing the subtree root replaced with
	 * 1 since it is the new root. This is equivalent to
	 * bfs' = bfs / sr + bfs % sr.
	 */
	/* mask off common bits */
	num_subtrees = 1 << top_height;
	bfs_number &= ~(subtree_root << subtree_depth);
	/* replace it with one */
	bfs_number |= 1 << subtree_depth;
	/*
	 * Now we need to count all the nodes before this one, then the
	 * position within this subtree. The number of siblings before
	 * this subtree root in the layout is the bottom k-1 bits of the
	 * subtree root.
	 */
	subtree_size = (1 << bottom_height) - 1;
	toptree_size = (1 << top_height) - 1;
	height = bottom_height;
	offset += toptree_size +
		(subtree_root & (num_subtrees - 1)) * subtree_size;
	// bfs_number--;  // (stupidity adjustment)
	// return prior_length + bfs_to_veb_recur(bfs_number, bottom_height);
	goto recurse;
}

void veb_build_cache() {
	should_build_cache = false;
	uint64_t offset = 0;
	for (uint64_t height = 1; ; height++) {
		cache_starts[height] = offset;
		for (uint64_t i = 0; i < (1 << height) - 1; i++) {
			uint64_t left, right;
			const veb_children children = veb_get_children(i, height);
			const veb_pointer l = children.left, r = children.right;

			if (!VP_PRESENT(l)) {
				uint64_t leaf_number = veb_get_leaf_index_of_leaf(i, height) * 2;
				assert(leaf_number + 1 < MAX_CACHED_NODE_INDEX);

				left = C(leaf_number);
				right = C(leaf_number + 1);
			} else {
				assert(l< MAX_CACHED_NODE_INDEX &&
						r < MAX_CACHED_NODE_INDEX);
				left = l;
				right = r;

				if (ISC(l) || ISC(r)) {
					log_info("cannot cache height %" PRIu64 ", "
							"too many nodes");
					goto done;
				}
			}

			const uint64_t li = offset + (i * 2);
			const uint64_t ri = offset + (i * 2) + 1;
			// printf("[%d] <= %d\n[%d] <= %d\n\n", li, left, ri, right);
			if (li >= CACHE_SIZE ||
					ri >= CACHE_SIZE) {
				log_info("cannot cache height %" PRIu64 ", "
						"cache is full", height);
				goto done;
			}

			assert(li < CACHE_SIZE && ri < CACHE_SIZE);
			cache[li] = left;
			cache[ri] = right;
		}
		offset += 2 * ((1 << height) - 1);
		max_cached_height = height;  // OK, cached
	}
done:
	log_info("VEB cache built for heights 1..%" PRIu64, max_cached_height);
}

veb_children veb_get_children(uint64_t node, uint8_t height) {
	// Build cache if it's dead.
	//if (should_build_cache) {
	//	veb_build_cache();
	//}

	uint64_t node_start = 0;
	veb_pointer leaf_source = VP_NO_NODE;
	uint64_t leaf_stride = 0;

	uint8_t bottom_height = hyperfloor8(height - 1);
	uint8_t top_height = height - bottom_height;
	while (height > 1) {
		//if (height <= max_cached_height) {
		//	const uint64_t c_start = cache_starts[height];
		//	const uint64_t LC = cache[c_start + node * 2];
		//	const uint64_t RC = cache[c_start + node * 2 + 1];

		//	if (ISC(LC)) {
		//		return (veb_children) {
		//			.left = VP_ADD(leaf_source, leaf_stride * UNC(LC)),
		//			.right = VP_ADD(leaf_source, leaf_stride * UNC(RC))
		//		};
		//	} else {
		//		return (veb_children) {
		//			.left = node_start + LC,
		//			.right = node_start + RC
		//		};
		//	}
		//}

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t nodes_in_bottom_block = m_exp2(bottom_height) - 1;

		if (node < nodes_in_top_block) {
			height = top_height;
			leaf_source = node_start + nodes_in_top_block;
			leaf_stride = nodes_in_bottom_block;

			bottom_height = hyperfloor8(height - 1);
			top_height = height - bottom_height;
		} else {
			node -= nodes_in_top_block;
			const uint64_t bottom_block_index = node / nodes_in_bottom_block;
			node_start += nodes_in_top_block + node - (node % nodes_in_bottom_block);
			node %= nodes_in_bottom_block;
			leaf_source = VP_ADD(leaf_source,
					(leaf_stride * bottom_block_index) << bottom_height);

			height = bottom_height;
			bottom_height >>= 1;
			top_height = bottom_height;
		}
	}
	return (veb_children) {
		.left = leaf_source,
		.right = VP_ADD(leaf_source, leaf_stride)
	};
}

bool veb_is_leaf(uint64_t node, uint64_t height) {
	uint64_t node_start = 0;
recursive_call:
	assert(height > 0);
	if (height == 1) {
		return true;
	} else {
		uint8_t bottom_height, top_height;
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

uint64_t veb_get_leaf_number(uint64_t leaf_index, uint64_t height) {
	// TODO: probably can be optimized
	uint64_t power = m_exp2(height - 2);
	uint64_t x = leaf_index;
	uint64_t ptr = 0;  // root
	while (!veb_is_leaf(ptr, height)) {
		veb_children children = veb_get_children(ptr, height);
		assert(VP_PRESENT(children.left) && VP_PRESENT(children.right));
		if (x / power == 0) {
			ptr = children.left;
		} else {
			ptr = children.right;
		}
		x %= power;
		power /= 2;
	}
	return ptr;
}

// Conceptually derived from build_veb_layout
uint64_t veb_get_leaf_index_of_leaf(uint64_t node, uint8_t height) {
	uint64_t offset = 0;
recursive_call:
	assert(height > 0);
	if (height == 1) {
		assert(node == 0);
		return offset;
	} else {
		uint8_t bottom_height, top_height;
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
