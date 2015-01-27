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
		for (uint64_t i = 0; i < (1ULL << height) - 1ULL; i++) {
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
				*left = veb_pointer_add(leaf_source, leaf_stride * UNC(LC));
				*right = veb_pointer_add(leaf_source, leaf_stride * UNC(RC));
				return;
			} else {
				left->present = true;
				left->node = node_start + LC;
				right->present = true;
				right->node = node_start + RC;
				return;
			}
		}

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t nodes_in_bottom_block = m_exp2(bottom_height) - 1;

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

		const uint64_t nodes_in_top_block = m_exp2(top_height) - 1;
		const uint64_t nodes_in_bottom_block =
			m_exp2(bottom_height) - 1;
		const uint64_t leaves_per_bottom_block = m_exp2(bottom_height);
		const uint64_t number_of_bottom_blocks = m_exp2(top_height);

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

struct level_data veb_get_level_data(uint64_t height, uint64_t level) {
	// log_info("finding level %" PRIu64 " in height=%" PRIu64,
	// 		level, height);
	assert(level > 0);
	uint64_t bottom_height, top_height;
	split_height(height, &bottom_height, &top_height);
	// log_info("height %" PRIu64 " => top=%" PRIu64 " bottom=%" PRIu64,
	// 		height, top_height, bottom_height);

	if (level < top_height) {
		return veb_get_level_data(top_height, level);
	} else if (level == top_height) {
		return (struct level_data) {
			.top_size = (1ULL << top_height) - 1,
			.bottom_size = (1ULL << bottom_height) - 1,
			.top_depth = 0
		};
	} else {
		struct level_data r = veb_get_level_data(bottom_height,
				level - top_height);
		return (struct level_data) {
			.top_size = r.top_size,
			.bottom_size = r.bottom_size,
			.top_depth = r.top_depth + top_height
		};
	}
}

void veb_prepare(uint64_t height, struct level_data* levels) {
	if (height >= 2) {
		for (uint64_t depth = 1; depth < height; depth++) {
			levels[depth] = veb_get_level_data(height, depth);
//			log_info("");
		}
	}
}

void drilldown_begin(struct drilldown_track* track) {
	track->pos[0] = 0;
	track->depth = 0;
	track->bfs = 0;
}

static void add_level(struct level_data* ld, struct drilldown_track* track) {
	++track->depth;
//	log_info("going to BFS=%" PRIu64 ".", track->bfs);
//	log_info("level data for level=%" PRIu64 ": top_size=%" PRIu64
//			" bottom_size=%" PRIu64 " top_depth=%" PRIu64,
//			track->depth,
//			ld[track->depth].top_size, ld[track->depth].bottom_size,
//			ld[track->depth].top_depth);
//	log_info("root of my top tree is at depth %" PRIu64, ld[track->depth].top_depth);
//	log_info("that means node %" PRIu64, track->pos[ld[track->depth].top_depth]);
//	log_info("I am bottom tree number %" PRIu64, (track->bfs + 1) & ld[track->depth].top_size);
	// TODO: shift and minus, not multiply; also, logs
	track->pos[track->depth] = track->pos[ld[track->depth].top_depth] +
		ld[track->depth].top_size +
		((track->bfs + 1) & ld[track->depth].top_size) * ld[track->depth].bottom_size;
}

void drilldown_go_left(struct level_data* ld, struct drilldown_track* track) {
	track->bfs = (track->bfs << 1ULL) + 1;
	add_level(ld, track);
}

void drilldown_go_right(struct level_data* ld, struct drilldown_track* track) {
	track->bfs = (track->bfs << 1ULL) + 2;
	add_level(ld, track);
}

// Inverse of drilldown_go_(left|right)
void drilldown_go_up(struct drilldown_track* track) {
	track->depth--;
	if (track->bfs % 2) {
		track->bfs = (track->bfs - 1) >> 1ULL;
	} else {
		track->bfs = (track->bfs - 2) >> 1ULL;
	}
}
