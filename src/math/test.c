#include "math.h"
#include <assert.h>

static void test_floor_log2() {
	assert(floor_log2(1) == 0);
	assert(floor_log2(2) == 1);
	assert(floor_log2(3) == 1);
	assert(floor_log2(4) == 2);
	assert(floor_log2(6) == 2);
	assert(floor_log2(134) == 7);
}

static void test_exact_log2() {
	assert(floor_log2(1) == 0);
	assert(floor_log2(2) == 1);
	assert(floor_log2(4) == 2);
	assert(floor_log2(256) == 8);
}

void test_math() {
	test_floor_log2();
	test_exact_log2();

	assert(hyperfloor(1) == 1);
	assert(hyperfloor(187) == 128);
	assert(hyperfloor(67710) == 65536);

	assert(is_pow2(1));
	assert(is_pow2(4096));
	assert(!is_pow2(7));
	assert(!is_pow2(9999));
}
