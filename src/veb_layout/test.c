#include "veb_layout/test.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include "log/log.h"
#include "veb_layout/veb_layout.h"

const uint64_t NOTHING = 0xDEADBEEF;

typedef struct {
	uint64_t left, right;
} persisted_node;

static persisted_node NODE_POOL[256];
uint64_t HEIGHT;

static uint64_t veb_pointer_to_id(veb_pointer ptr) {
	if (ptr.present) {
		return ptr.node;
	} else {
		return NOTHING;
	}
}

static void set_node(void* _veb_buffer, uint64_t node,
		veb_pointer left, veb_pointer right) {
	persisted_node* pool = _veb_buffer;
	pool[node].left = veb_pointer_to_id(left);
	pool[node].right = veb_pointer_to_id(right);
}

#define check_nonleaf(veb_index) do { \
	assert(!veb_is_leaf(veb_index, HEIGHT)); \
} while (0)

#define check_leaf_number(leaf_index,veb_index) do { \
	assert(veb_get_leaf_number(leaf_index, HEIGHT) == veb_index); \
	assert(veb_get_leaf_index_of_leaf(veb_index, HEIGHT) == leaf_index); \
	assert(veb_is_leaf(veb_index, HEIGHT)); \
} while (0)

#define check(index,left_n,right_n) do { \
	const persisted_node node = NODE_POOL[index]; \
	assert(node.left == left_n && node.right == right_n); \
	veb_pointer lx, rx; \
	veb_get_children(index, HEIGHT, &lx, &rx); \
	assert(veb_pointer_to_id(lx) == left_n && veb_pointer_to_id(rx) == right_n); \
} while (0)

static void build_with_height(uint64_t height) {
	build_veb_layout(height, 0, set_node, NODE_POOL,
			(veb_pointer) { .present = false }, 0);
}

static void test_1() {
	HEIGHT = 1;
	build_with_height(1);
	check(0, NOTHING, NOTHING); check_leaf_number(0, 0);
}

static void test_2() {
	HEIGHT = 2;
	build_with_height(2);
	check(0, 1, 2); check_nonleaf(0);
	check(1, NOTHING, NOTHING); check_leaf_number(0, 1);
	check(2, NOTHING, NOTHING); check_leaf_number(1, 2);
}

static void test_3() {
	HEIGHT = 3;
	build_with_height(3);
	check(0, 1, 4); check_nonleaf(0);
	check(1, 2, 3); check_nonleaf(1);
	check(2, NOTHING, NOTHING); check_leaf_number(0, 2);
	check(3, NOTHING, NOTHING); check_leaf_number(1, 3);
	check(4, 5, 6); check_nonleaf(4);
	check(5, NOTHING, NOTHING); check_leaf_number(2, 5);
	check(6, NOTHING, NOTHING); check_leaf_number(3, 6);
}

static void test_4() {
	HEIGHT = 4;
	build_with_height(4);
	check(0, 1, 2); check_nonleaf(0);
	check(1, 3, 6); check_nonleaf(1);
	check(2, 9, 12); check_nonleaf(2);
	check(3, 4, 5); check_nonleaf(3);
	check(4, NOTHING, NOTHING); check_leaf_number(0, 4);
	check(5, NOTHING, NOTHING); check_leaf_number(1, 5);
	check(6, 7, 8); check_nonleaf(6);
	check(7, NOTHING, NOTHING); check_leaf_number(2, 7);
	check(8, NOTHING, NOTHING); check_leaf_number(3, 8);
	check(9, 10, 11); check_nonleaf(9);
	check(10, NOTHING, NOTHING); check_leaf_number(4, 10);
	check(11, NOTHING, NOTHING); check_leaf_number(5, 11);
	check(12, 13, 14); check_nonleaf(12);
	check(13, NOTHING, NOTHING); check_leaf_number(6, 13);
	check(14, NOTHING, NOTHING); check_leaf_number(7, 14);
}

static void test_5() {
	HEIGHT = 5;
	build_with_height(5);
	check(0, 1, 16); check_nonleaf(0);
	check(1, 2, 3); check_nonleaf(1);
	check(2, 4, 7); check_nonleaf(2);
	check(3, 10, 13); check_nonleaf(3);
	check(4, 5, 6); check_nonleaf(4);
	check(5, NOTHING, NOTHING); check_leaf_number(0, 5);
	check(6, NOTHING, NOTHING); check_leaf_number(1, 6);
	check(7, 8, 9); check_nonleaf(7);
	check(8, NOTHING, NOTHING); check_leaf_number(2, 8);
	check(9, NOTHING, NOTHING); check_leaf_number(3, 9);
	check(10, 11, 12); check_nonleaf(10);
	check(11, NOTHING, NOTHING); check_leaf_number(4, 11);
	check(12, NOTHING, NOTHING); check_leaf_number(5, 12);
	check(13, 14, 15); check_nonleaf(13);
	check(14, NOTHING, NOTHING); check_leaf_number(6, 14);
	check(15, NOTHING, NOTHING); check_leaf_number(7, 15);
	check(16, 17, 18); check_nonleaf(16);
	check(17, 19, 22); check_nonleaf(17);
	check(18, 25, 28); check_nonleaf(18);
	check(19, 20, 21); check_nonleaf(19);
	check(20, NOTHING, NOTHING); check_leaf_number(8, 20);
	check(21, NOTHING, NOTHING); check_leaf_number(9, 21);
	check(22, 23, 24); check_nonleaf(22);
	check(23, NOTHING, NOTHING); check_leaf_number(10, 23);
	check(24, NOTHING, NOTHING); check_leaf_number(11, 24);
	check(25, 26, 27); check_nonleaf(25);
	check(26, NOTHING, NOTHING); check_leaf_number(12, 26);
	check(27, NOTHING, NOTHING); check_leaf_number(13, 27);
	check(28, 29, 30); check_nonleaf(28);
	check(29, NOTHING, NOTHING); check_leaf_number(14, 29);
	check(30, NOTHING, NOTHING); check_leaf_number(15, 30);
}

static void test_drilldown_level_data() {
	veb_level_data r;
	// 0 = forbidden (root) level
	r = veb_get_level_data(2, 1);
	assert(r.top_size == 1 && r.bottom_size == 1 && r.top_depth == 0);

	// 0 = forbidden (root) level
	r = veb_get_level_data(3, 1);
	assert(r.top_size == 1 && r.bottom_size == 3 && r.top_depth == 0);
	r = veb_get_level_data(3, 2);
	assert(r.top_size == 1 && r.bottom_size == 1 && r.top_depth == 1);

	// 0 = forbidden (root) level
	//
	// 0
	// 1 1
	// ===
	// 2 2 2 2
	// 3 3 3 3 3 3 3 3
	r = veb_get_level_data(4, 1);
	assert(r.top_size == 1 && r.bottom_size == 1 && r.top_depth == 0);
	r = veb_get_level_data(4, 2);
	assert(r.top_size == 3 && r.bottom_size == 3 && r.top_depth == 0);
	r = veb_get_level_data(4, 3);
	assert(r.top_size == 1 && r.bottom_size == 1 && r.top_depth == 2);

	//   0
	//   ===
	//   ===
	//   1 1
	//   2 2 2 2
	//   ===============
	//   3 3 3 3 3 3 3 3
	//   4 4 4 4 4 4 4 4 4 4 4 4 4 4 4 4
	//
	// 0 = forbidden (root) level
	r = veb_get_level_data(5, 1);
	assert(r.top_size == 1 && r.bottom_size == 15 && r.top_depth == 0);
	r = veb_get_level_data(5, 2);
	assert(r.top_size == 1 && r.bottom_size == 1 && r.top_depth == 1);
	r = veb_get_level_data(5, 3);
	assert(r.top_size == 3 && r.bottom_size == 3 && r.top_depth == 1);
	r = veb_get_level_data(5, 4);
	assert(r.top_size == 1 && r.bottom_size == 1 && r.top_depth == 3);
}

static void test_drilldown_small() {
	struct drilldown_track track;
	veb_level_data levels[5 + 1];
	veb_prepare(5, levels);

	drilldown_begin(&track);
	assert(track.depth == 0 && track.pos[track.depth] == 0 && track.bfs == 0);

	drilldown_go_right(levels, &track);
	assert(track.depth == 1 && track.pos[track.depth] == 16 && track.bfs == 2);

	drilldown_go_left(levels, &track);
	assert(track.depth == 2 && track.pos[track.depth] == 17 && track.bfs == 5);

	drilldown_go_right(levels, &track);
	assert(track.depth == 3 && track.pos[track.depth] == 22 && track.bfs == 12);

	drilldown_go_left(levels, &track);
	assert(track.depth == 4 && track.pos[track.depth] == 23 && track.bfs == 25);
}

static void test_drilldown_integration() {
	struct drilldown_track track;
	veb_level_data levels[50 + 1];

	for (uint64_t iteration = 0; iteration < 10000; iteration++) {
		uint64_t height = rand() % 50 + 1;
		veb_prepare(height, levels);

		drilldown_begin(&track);
		uint64_t reference_node = 0;

		for (uint64_t h = 0; h < height; h++) {
			assert(reference_node == track.pos[track.depth]);
			if (h < height - 1) {
				veb_pointer left, right;
				veb_get_children(reference_node, height, &left, &right);

				if (rand() % 2 == 0) {
					drilldown_go_left(levels, &track);
					reference_node = left.node;
				} else {
					drilldown_go_right(levels, &track);
					reference_node = right.node;
				}
			}
		}
	}
}

static void test_drilldown() {
	test_drilldown_level_data();
	test_drilldown_small();
	test_drilldown_integration();
}

void test_veb_layout() {
	test_1();
	test_2();
	test_3();
	test_4();
	test_5();

	test_drilldown();
}
