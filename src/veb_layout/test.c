#include "test.h"
#include "veb_layout.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include "../log/log.h"

static veb_node veb_buffer[256];

const int STRIDE = 5;

static veb_node* MOCK(uint64_t n) {
	return (veb_node*) (0x1000 + n * sizeof(veb_node) * STRIDE);
}

static veb_node* PTR(uint64_t n) {
	return &veb_buffer[n];
}

static void base_and_diff(const char* base, uint64_t diff, char* output) {
	if (diff > 0) {
		sprintf(output, "%s+%" PRIu64, base, diff);
	} else {
		strcpy(output, base);
	}
}

static void describe(const veb_node* node, char* description) {
	char buffer[20];
	if (((uint64_t) node) < 0x10000) {
		uint64_t diff = ((uint64_t) node) - ((uint64_t) MOCK(0));
		sprintf(buffer, "MOCK(%" PRIu64 ")",
				diff / (sizeof(veb_node) * STRIDE));
		base_and_diff(buffer, diff % (sizeof(veb_node) * STRIDE),
				description);
	} else if (node >= veb_buffer && node < veb_buffer + sizeof(veb_buffer)) {
		uint64_t diff = ((uint64_t) node) - ((uint64_t) veb_buffer);
		sprintf(buffer, "veb_buffer[%" PRIu64 "]",
				diff / sizeof(veb_node));
		base_and_diff(buffer, diff % sizeof(veb_node),
				description);
	} else {
		sprintf(description, "%p", node);
	}
}

#define check(index,left_n,right_n) do { \
	char should[100], found[100]; \
	const veb_node node = veb_buffer[index]; \
	if (node.left != left_n) { \
		describe(node.left, found); \
		describe(left_n, should); \
		log_fatal("node %d has left wrong (is %s, should be %s)", \
				index, found, should); \
	} \
	if (node.right != right_n) { \
		describe(node.right, found); \
		describe(right_n, should); \
		log_fatal("node %d has right wrong (is %s, should be %s)", \
				index, found, should); \
	} \
} while (0)

static void test_1() {
	build_veb_layout(1, veb_buffer, MOCK(0), STRIDE);
	check(0, MOCK(0), MOCK(1));
}

static void test_2() {
	build_veb_layout(2, veb_buffer, MOCK(0), STRIDE);

	check(0, PTR(1), PTR(2));
	check(1, MOCK(0), MOCK(1));
	check(2, MOCK(2), MOCK(3));
}

static void test_3() {
	build_veb_layout(3, veb_buffer, MOCK(0), STRIDE);

	check(0, PTR(1), PTR(4));
	check(1, PTR(2), PTR(3));
	check(2, MOCK(0), MOCK(1));
	check(3, MOCK(2), MOCK(3));
	check(4, PTR(5), PTR(6));
	check(5, MOCK(4), MOCK(5));
	check(6, MOCK(6), MOCK(7));
}

static void test_4() {
	build_veb_layout(4, veb_buffer, MOCK(0), STRIDE);

	check(0, PTR(1), PTR(2));
	check(1, PTR(3), PTR(6));
	check(2, PTR(9), PTR(12));
	check(3, PTR(4), PTR(5));
	check(4, MOCK(0), MOCK(1));
	check(5, MOCK(2), MOCK(3));
	check(6, PTR(7), PTR(8));
	check(7, MOCK(4), MOCK(5));
	check(8, MOCK(6), MOCK(7));
	check(9, PTR(10), PTR(11));
	check(10, MOCK(8), MOCK(9));
	check(11, MOCK(10), MOCK(11));
	check(12, PTR(13), PTR(14));
	check(13, MOCK(12), MOCK(13));
	check(14, MOCK(14), MOCK(15));
}

void test_veb_layout() {
	test_1();
	test_2();
	test_3();
	test_4();
}
