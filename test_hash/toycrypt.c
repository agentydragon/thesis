#include "toycrypt.h"

static uint32_t oneway_fn(uint32_t a, uint64_t b) {
	uint32_t x = a, y = *((uint32_t*)(&b)), z = *((uint32_t*)(&b) + 1);
	return (x << 24) + (y << 16) + (z << 8) + x;
}

static uint64_t one_round(uint64_t message, uint64_t key) {
	uint32_t L1 = *(uint32_t*)(&message), L2 = *((uint32_t*)(&message) + 1);
	uint32_t R1 = L2 ^ oneway_fn(L1, key) , R2 = L1;
	uint64_t out;
	*(uint32_t*)(&out) = R1;
	*((uint32_t*)(&out) + 1) = R2;
	return out;
}

uint64_t toycrypt(uint64_t message, uint64_t key) {
	for (uint8_t i = 0; i < 3; i++) {
		message = one_round(message, key + i);
	}
	return message;
}
