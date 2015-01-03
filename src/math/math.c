#include "math.h"
#include "../log/log.h"

#include <inttypes.h>

#define CLZ64(x) (__builtin_clzll(x))

static uint64_t m_exp2(uint64_t x) {
	return 1ULL << x;
}

uint8_t ceil_log2(uint64_t x) {
	// TODO: optimize
	if (x == 0) return 0;
	uint64_t exp_l = 1;
	uint8_t l = 0;
	for (; exp_l < x; exp_l *= 2, ++l);
	return l;
}

uint8_t floor_log2(uint64_t x) {
	// TODO: optimize;
	if (x == 0) return 0;
	uint64_t pow, exp;
	for (pow = 1, exp = 0; pow <= x; pow *= 2, ++exp);
	return exp - 1;
}

uint64_t hyperfloor(uint64_t x) {
	// return m_exp2(floor_log2(x));
	return 1ULL << (sizeof(uint64_t) * 8 - 1 - CLZ64(x));
}

uint64_t ceil_div(uint64_t a, uint64_t b) {
	return (a / b) + (a % b > 0) ? 1 : 0;
}

uint8_t exact_log2(uint64_t x) {
	CHECK(is_pow2(x), "Exact log2 of %" PRIu64 " doesn't exist.");
	return floor_log2(x);
}
