#include "math/math.h"

#include <inttypes.h>

#include "log/log.h"

#define CLZ64(x) (__builtin_clzll(x))

uint64_t ceil_div(uint64_t a, uint64_t b) {
	return (a / b) + (a % b > 0) ? 1 : 0;
}

uint8_t exact_log2(uint64_t x) {
	CHECK(is_pow2(x), "Exact log2 of %" PRIu64 " doesn't exist.");
	return floor_log2(x);
}
