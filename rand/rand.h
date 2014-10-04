#ifndef RAND_H
#define RAND_H

#include <stdint.h>

typedef struct {
	uint64_t state;
} rand_generator;

uint64_t rand_next(rand_generator* generator, uint64_t max);

#endif
