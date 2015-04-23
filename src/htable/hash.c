#include "htable/hash.h"

#include <assert.h>

#include "math/math.h"
#include "log/log.h"

uint64_t fnv_hash(uint64_t key, uint64_t hash_max) {
	uint64_t result = 0xcbf29ce484222325;
	for (int i = 0; i < 8; i++) {
		result *= 0x100000001b3;
		result ^= key & 0xFF;
		key >>= 8;
	}
	return result % hash_max;
}

// Simple tabulation hashing.

void sth_init(sth* this, uint64_t hash_max, rand_generator* rand) {
	this->hash_max = hash_max;
	// for (uint64_t i = 0; i < sizeof(uint64_t); ++i) {
	// 	for (uint64_t j = 0; j < 256; ++j) {
	// 		this->table[i][j] = rand_next64(rand);
	// 	}
	// }
	for (uint64_t i = 0; i < sizeof(uint64_t) * 2; ++i) {
		for (uint64_t j = 0; j < 16; ++j) {
			this->table[i][j] = rand_next64(rand);
		}
	}
	ASSERT(is_pow2(hash_max));
}

uint64_t sth_hash(const sth* this, uint64_t key) {
	uint64_t result = 0;
	// for (uint64_t i = 0; i < sizeof(uint64_t); ++i) {
	// 	const uint8_t byte = (key & (0xFFULL << (i * 8))) >> (i * 8);
	// 	result ^= this->table[i][byte];
	// }
	for (uint64_t i = 0; i < sizeof(uint64_t); ++i) {
		const uint8_t byte = (key & (0xFFULL << (i * 8))) >> (i * 8);
		const uint8_t low = byte & 0x0F;
		const uint8_t high = (byte & 0xF0) >> 4;
		result ^= this->table[i * 2][high];
		result ^= this->table[i * 2 + 1][low];
	}
	// Compute result % this->hash_max, but slightly more efficiently
	// (assuming that this->hash_max is a power of 2).
	return result & (this->hash_max - 1);
}
