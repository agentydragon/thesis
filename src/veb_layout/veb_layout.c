#include "veb_layout.h"
#include <assert.h>
#include <inttypes.h>
#include "../math/math.h"
#include "../log/log.h"

#define EXP2(x) (1ULL << (x))

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

		const uint64_t nodes_in_top_block = EXP2(top_height) - 1;
		const uint64_t number_of_bottom_blocks = EXP2(top_height);
		const uint64_t nodes_in_bottom_block = EXP2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = EXP2(bottom_height);

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

void veb_build_cache() {
	should_build_cache = false;
	uint64_t offset = 0;
	for (uint64_t height = 1; ; height++) {
		cache_starts[height] = offset;
		for (uint64_t i = 0; i < (1 << height) - 1; i++) {
			uint64_t left, right;
			veb_pointer l, r;
			veb_get_children(i, height, &l, &r);

			if (!l.present) {
				uint64_t leaf_number = veb_get_leaf_index_of_leaf(i, height) * 2;
				assert(leaf_number + 1 < MAX_CACHED_NODE_INDEX);

				left = C(leaf_number);
				right = C(leaf_number + 1);
			} else {
				assert(l.node < MAX_CACHED_NODE_INDEX &&
						r.node < MAX_CACHED_NODE_INDEX);
				left = l.node;
				right = r.node;

				if (ISC(l.node) || ISC(r.node)) {
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

void veb_get_children(uint64_t node, uint64_t height,
		veb_pointer* left, veb_pointer* right) {
	// Build cache if it's dead.
	if (should_build_cache) {
		veb_build_cache();
	}

	uint64_t node_start = 0;
	veb_pointer leaf_source = (veb_pointer) {
		.present = false,
		.node = 0
	};
	uint64_t leaf_stride = 0;

	uint8_t bottom_height = hyperfloor(height - 1);
	uint8_t top_height = height - bottom_height;
	while (height > 1) {
		if (height <= max_cached_height) {
			const uint64_t c_start = cache_starts[height];
			const uint64_t LC = cache[c_start + node * 2];
			const uint64_t RC = cache[c_start + node * 2 + 1];

			if (ISC(LC)) {
				*left = veb_pointer_add(leaf_source, leaf_stride * (UNC(LC)));
				*right = veb_pointer_add(leaf_source, leaf_stride * (UNC(RC)));
				return;
			} else {
				left->present = true;
				left->node = node_start + LC;
				right->present = true;
				right->node = node_start + RC;
				return;
			}
		}

		const uint64_t nodes_in_top_block = EXP2(top_height) - 1;
		const uint64_t nodes_in_bottom_block = EXP2(bottom_height) - 1;

		if (node < nodes_in_top_block) {
			height = top_height;
			leaf_source.present = true;
			leaf_source.node = node_start + nodes_in_top_block;
			leaf_stride = nodes_in_bottom_block;

			bottom_height = hyperfloor(height - 1);
			top_height = height - bottom_height;
		} else {
			node -= nodes_in_top_block;
			const uint64_t bottom_block_index = node / nodes_in_bottom_block;
			node_start += nodes_in_top_block + node - (node % nodes_in_bottom_block);
			node %= nodes_in_bottom_block;
			leaf_source.node += (leaf_stride * bottom_block_index) << bottom_height;

			height = bottom_height;
			bottom_height >>= 1;
			top_height = bottom_height;
		}
	}
	*left = leaf_source;
	*right = veb_pointer_add(leaf_source, leaf_stride);
}

// Conceptually derived from build_veb_layout.
// This function is easier to understand with recursion, but to get better
// speed, we change it manually to be tail-recursive.
static void __attribute__((unused)) veb_get_children_UNCACHED(uint64_t node, uint64_t height,
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

		const uint64_t nodes_in_top_block = EXP2(top_height) - 1;
		const uint64_t nodes_in_bottom_block = EXP2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = EXP2(bottom_height);
		const uint64_t number_of_bottom_blocks = EXP2(top_height);

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

		const uint64_t nodes_in_top_block = EXP2(top_height) - 1;
		const uint64_t number_of_bottom_blocks = EXP2(top_height);
		const uint64_t nodes_in_bottom_block = EXP2(bottom_height) - 1;

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
	uint64_t power = EXP2(height - 2);
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

		const uint64_t nodes_in_top_block = EXP2(top_height) - 1;
		const uint64_t nodes_in_bottom_block = EXP2(bottom_height) - 1;

		// If we ever go up, it's not a leaf anymore.
		assert(node >= nodes_in_top_block);

		const uint64_t bottom_block_index = (node - nodes_in_top_block) / nodes_in_bottom_block;
		const uint64_t bottom_block_node = (node - nodes_in_top_block) % nodes_in_bottom_block;

		offset += bottom_block_index << (bottom_height - 1);
		node = bottom_block_node;
		height = bottom_height;
		goto recursive_call;
	}
}

void veb_drilldown_start(uint64_t height, struct drilldown_scratchpad* scratch) {
	scratch->current = &scratch->stack[0];
	scratch->current->min_node = 0;
	scratch->current->max_node = EXP2(height) - 1;
	scratch->current->leaf_source.present = false;
	scratch->current->leaf_source.node = 0;
	scratch->current->leaf_stride = 0;
	scratch->current->height = height;
}

void veb_drilldown_get_children(uint64_t node, uint64_t height,
		veb_pointer* left, veb_pointer* right,
		struct drilldown_scratchpad* scratch) {
	while (scratch->current->max_node <= node ||
			scratch->current->min_node > node) {
		--scratch->current;
	}

	// Until you find the node you're looking for...
	while (scratch->current->max_node > scratch->current->min_node + 1) {
		assert(node >= scratch->current->min_node &&
				node < scratch->current->max_node);
		uint64_t bottom_height, top_height;
		split_height(scratch->current->height,
				&bottom_height, &top_height);

		const uint64_t nodes_in_top_block = EXP2(top_height) - 1;
		const uint64_t nodes_in_bottom_block = EXP2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = EXP2(bottom_height);
		const uint64_t number_of_bottom_blocks = EXP2(top_height);

		struct drilldown_block* next = scratch->current + 1;
		if (node < scratch->current->min_node + nodes_in_top_block) {
			next->min_node = scratch->current->min_node;
			next->max_node = scratch->current->min_node +
				nodes_in_top_block;
			next->leaf_source.present = true;
			next->leaf_source.node = scratch->current->min_node +
				nodes_in_top_block;
			next->leaf_stride = nodes_in_bottom_block;
			next->height = top_height;
		} else {
			const uint64_t bottom_block_index =
					(node - scratch->current->min_node -
					 nodes_in_top_block) / nodes_in_bottom_block;
			assert(bottom_block_index < number_of_bottom_blocks);
			next->min_node = scratch->current->min_node + nodes_in_top_block +
				bottom_block_index * nodes_in_bottom_block;
			next->max_node = scratch->current->min_node + nodes_in_top_block +
				(bottom_block_index + 1) * nodes_in_bottom_block;
			next->leaf_source.present = scratch->current->leaf_source.present;
			next->leaf_source.node = scratch->current->leaf_source.node +
				scratch->current->leaf_stride * leaves_per_bottom_block *
				bottom_block_index;
			next->leaf_stride = scratch->current->leaf_stride;
			next->height = bottom_height;
		}
		++scratch->current;
	}

	*left = scratch->current->leaf_source;
	*right = veb_pointer_add(scratch->current->leaf_source,
			scratch->current->leaf_stride);
}
