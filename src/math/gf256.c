#include "math/gf256.h"

gf256 gf256_plus(gf256 a, gf256 b) {
	return a ^ b;
}

gf256 gf256_minus(gf256 x) {
	return ~x;
}

gf256 gf256_mult(gf256 a, gf256 b) {
	// if (PRECOMPUTED) {
	// 	return precomputed_mult[a * 256 + b];
	// }
	gf256 result = 0;
	for (uint8_t i = 0; i < 8; ++i) {
		if (a & 1) {
			result ^= b;
		}
		if (b & 0b10000000) {
			// Shifting to the left will create x^8.
			b <<= 1;
			// Replace x^8 with x^4 + x^3 + x + 1.
			b ^= 0b00011011;
		} else {
			b <<= 1;
		}
		a >>= 1;
	}
	return result;
}

/*
gf256 gf256_inverse(gf256 x) {
	// XXX: Too lazy to implement extended Euclid's algorithm...
	ASSERT(PRECOMPUTED);
}
*/
