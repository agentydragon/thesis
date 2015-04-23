#ifndef MATH_GF256_H
#define MATH_GF256_H

#include <stdint.h>

// Basic operations in GF(256).
// The irreducible polynomial used is x^8 + x^4 + x^3 + x + 1.

typedef uint8_t gf256;

gf256 gf256_plus(gf256 a, gf256 b);
gf256 gf256_minus(gf256 x);
gf256 gf256_mult(gf256 a, gf256 b);

gf256 gf256_precompute();

#endif
