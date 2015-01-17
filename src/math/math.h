#ifndef PRVAK_MATH_H
#define PRVAK_MATH_H

#include <stdbool.h>
#include <stdint.h>

#define CLZ64(x) (__builtin_clzll(x))
#define CTZ64(x) (__builtin_ctzll(x))
#define floor_log2_64(x) (sizeof(uint64_t) * 8 - 1 - CLZ64(x))
#define hyperfloor64(x) (1ULL << floor_log2_64(x))

#define CTZ8(x) (__builtin_ctz(x))
#define CLZ8(x) (__builtin_clz(x))
#define floor_log2_8(x) (sizeof(uint8_t) * 8 - 1 - CLZ8(x))
#define hyperfloor8(x) (1ULL << floor_log2_64(x))

#define is_pow2(x) (((x) & ((x) - 1)) == 0)

uint8_t ceil_log2(uint64_t x);
uint8_t exact_log2(uint64_t x);
uint64_t ceil_div(uint64_t a, uint64_t b);

#endif
