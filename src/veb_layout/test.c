#include "test.h"
#include "veb_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include "../log/log.h"

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

#define check(index,left_n,right_n) do { \
	const persisted_node node = NODE_POOL[index]; \
	assert(node.left == left_n && node.right == right_n); \
	const veb_pointer left_found = veb_get_left(index, HEIGHT, \
			0, \
			(veb_pointer) { .present = false }, 0); \
	const uint64_t left_fn = veb_pointer_to_id(left_found); \
	if (left_fn != left_n) { \
		log_fatal("height %" PRIu64 " node %" PRIu64 " should " \
				"have left %" PRIu64 ", has %" PRIu64, \
				HEIGHT, index, left_n, left_fn); \
	} \
	const veb_pointer right_found = veb_get_right(index, HEIGHT, \
			0, \
			(veb_pointer) { .present = false }, 0); \
	const uint64_t right_fn = veb_pointer_to_id(right_found); \
	if (right_fn != right_n) { \
		log_fatal("height %" PRIu64 " node %" PRIu64 " should " \
				"have right %" PRIu64 ", has %" PRIu64, \
				HEIGHT, index, right_n, right_fn); \
	} \
} while (0)

	// assert(veb_pointer_to_id(left_found) == left_n); \


static void build_with_height(uint64_t height) {
	build_veb_layout(height, 0, set_node, NODE_POOL,
			(veb_pointer) { .present = false }, 0);
}

static void test_1() {
	HEIGHT = 1;
	build_with_height(1);
	check(0, NOTHING, NOTHING);
}

static void test_2() {
	HEIGHT = 2;
	build_with_height(2);
	check(0, 1, 2);
	check(1, NOTHING, NOTHING);
	check(2, NOTHING, NOTHING);
}

static void test_3() {
	HEIGHT = 3;
	build_with_height(3);
	check(0, 1, 4);
	check(1, 2, 3);
	check(2, NOTHING, NOTHING);
	check(3, NOTHING, NOTHING);
	check(4, 5, 6);
	check(5, NOTHING, NOTHING);
	check(6, NOTHING, NOTHING);
}

static void test_4() {
	HEIGHT = 4;
	build_with_height(4);
	check(0, 1, 2);
	check(1, 3, 6);
	check(2, 9, 12);
	check(3, 4, 5);
	check(4, NOTHING, NOTHING);
	check(5, NOTHING, NOTHING);
	check(6, 7, 8);
	check(7, NOTHING, NOTHING);
	check(8, NOTHING, NOTHING);
	check(9, 10, 11);
	check(10, NOTHING, NOTHING);
	check(11, NOTHING, NOTHING);
	check(12, 13, 14);
	check(13, NOTHING, NOTHING);
	check(14, NOTHING, NOTHING);
}

static void test_5() {
	HEIGHT = 5;
	build_with_height(5);
	check(0, 1, 16);
	check(1, 2, 3);
	check(2, 4, 7);
	check(3, 10, 13);
	check(4, 5, 6);
	check(5, NOTHING, NOTHING);
	check(6, NOTHING, NOTHING);
	check(7, 8, 9);
	check(8, NOTHING, NOTHING);
	check(9, NOTHING, NOTHING);
	check(10, 11, 12);
	check(11, NOTHING, NOTHING);
	check(12, NOTHING, NOTHING);
	check(13, 14, 15);
	check(14, NOTHING, NOTHING);
	check(15, NOTHING, NOTHING);
	check(16, 17, 18);
	check(17, 19, 22);
	check(18, 25, 28);
	check(19, 20, 21);
	check(20, NOTHING, NOTHING);
	check(21, NOTHING, NOTHING);
	check(22, 23, 24);
	check(23, NOTHING, NOTHING);
	check(24, NOTHING, NOTHING);
	check(25, 26, 27);
	check(26, NOTHING, NOTHING);
	check(27, NOTHING, NOTHING);
	check(28, 29, 30);
	check(29, NOTHING, NOTHING);
	check(30, NOTHING, NOTHING);
}

void test_veb_layout() {
	test_1();
	test_2();
	test_3();
	test_4();
	test_5();
}
