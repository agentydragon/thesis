#ifndef UTIL_CONSUME_H
#define UTIL_CONSUME_H

#include <stdbool.h>
#include <stdint.h>

// consume* forces a value to never be optimized away.

static inline void consume64(uint64_t x) {
	__asm volatile ("" : : "r" (x));
}

static inline void consume_bool(bool x) {
	__asm volatile ("" : : "r" (x));
}

#endif
