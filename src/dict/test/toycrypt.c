#include "dict/test/toycrypt.h"

static uint32_t oneway_fn(uint32_t a, uint64_t b) {
	// A variant of the FNV hashing function.
	const uint32_t basis = 2166136261LL, prime = 16777619LL;
	return (((basis * prime) ^ a) * prime) ^ b;
}

static uint64_t one_round(uint64_t message, uint64_t key) {
	uint32_t L1 = (message & 0xFFFFFFFF00000000LL) >> 32LL,
		 L2 = (message & 0x00000000FFFFFFFFLL);
	uint32_t R1 = L2 ^ oneway_fn(L1, key), R2 = L1;
	return (((uint64_t) R1) << 32LL) | (uint64_t) R2;
}

uint64_t toycrypt(uint64_t message, uint64_t key) {
	for (uint8_t i = 0; i < 3; i++) {
		message = one_round(message, key + i);
	}
	return message;
}
