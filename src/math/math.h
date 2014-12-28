#ifndef PRVAK_MATH_H
#define PRVAK_MATH_H

#include <stdbool.h>
#include <stdint.h>

bool is_pow2(uint64_t x);
uint8_t ceil_log2(uint64_t x);
uint8_t floor_log2(uint64_t x);
uint8_t exact_log2(uint64_t x);
uint64_t ceil_div(uint64_t a, uint64_t b);
uint64_t closest_pow2_floor(uint64_t x);

#endif
