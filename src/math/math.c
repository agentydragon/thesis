#include "math.h"

static uint64_t m_exp2(uint64_t x) {
	uint64_t log, n;
	for (log = 0, n = 1; log < x; n *= 2, ++log);
	return n;
}

bool is_pow2(uint64_t x) {
	// TODO: optimize
	return m_exp2(floor_log2(x)) == x;
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

uint64_t closest_pow2_floor(uint64_t x) {
	return m_exp2(floor_log2(x));
}

uint64_t ceil_div(uint64_t a, uint64_t b) {
	return (a / b) + (a % b > 0) ? 1 : 0;
}
