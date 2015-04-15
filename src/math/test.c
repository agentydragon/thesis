#include "math.h"
#include <assert.h>

static void test_floor_log2(void) {
	assert(floor_log2(1) == 0);
	assert(floor_log2(2) == 1);
	assert(floor_log2(3) == 1);
	assert(floor_log2(4) == 2);
	assert(floor_log2(6) == 2);
	assert(floor_log2(134) == 7);
}

static void test_ceil_log2(void) {
	assert(ceil_log2(1) == 0);
	assert(ceil_log2(2) == 1);
	assert(ceil_log2(3) == 2);
	assert(ceil_log2(4) == 2);
	assert(ceil_log2(6) == 3);
	assert(ceil_log2(134) == 8);
}

static void test_exact_log2(void) {
	assert(floor_log2(1) == 0);
	assert(floor_log2(2) == 1);
	assert(floor_log2(4) == 2);
	assert(floor_log2(256) == 8);
}

static void test_hyperfloor(void) {
	assert(hyperfloor(1) == 1);
	assert(hyperfloor(187) == 128);
	assert(hyperfloor(67710) == 65536);
	assert(hyperfloor(0x10000000003124) == 0x10000000000000);
}

void test_math(void) {
	test_floor_log2();
	test_ceil_log2();
	test_exact_log2();
	test_hyperfloor();

	assert(is_pow2(1));
	assert(is_pow2(4096));
	assert(!is_pow2(7));
	assert(!is_pow2(9999));
}
