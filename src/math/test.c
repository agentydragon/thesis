#include "math.h"
#include <assert.h>

void test_math() {
	assert(floor_log2(1) == 0);
	assert(floor_log2(2) == 1);
	assert(floor_log2(3) == 1);
	assert(floor_log2(4) == 2);
	assert(floor_log2(6) == 2);
	assert(floor_log2(134) == 7);

	assert(closest_pow2_floor(1) == 1);
	assert(closest_pow2_floor(187) == 128);
	assert(closest_pow2_floor(67710) == 65536);

	assert(is_pow2(1));
	assert(is_pow2(4096));
	assert(!is_pow2(7));
	assert(!is_pow2(9999));
}
