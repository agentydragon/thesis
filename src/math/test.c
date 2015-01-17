#include "math.h"
#include <assert.h>

static void test_floor_log2_64() {
	assert(floor_log2_64(1) == 0);
	assert(floor_log2_64(2) == 1);
	assert(floor_log2_64(3) == 1);
	assert(floor_log2_64(4) == 2);
	assert(floor_log2_64(6) == 2);
	assert(floor_log2_64(134) == 7);
}

static void test_exact_log2() {
	assert(exact_log2(1) == 0);
	assert(exact_log2(2) == 1);
	assert(exact_log2(4) == 2);
	assert(exact_log2(256) == 8);
}

static void test_hyperfloor64() {
	assert(hyperfloor64(1) == 1);
	assert(hyperfloor64(187) == 128);
	assert(hyperfloor64(67710) == 65536);
	assert(hyperfloor64(0x10000000003124) == 0x10000000000000);
}

void test_math() {
	test_floor_log2_64();
	test_exact_log2();
	test_hyperfloor64();

	assert(is_pow2(1));
	assert(is_pow2(4096));
	assert(!is_pow2(7));
	assert(!is_pow2(9999));
}
