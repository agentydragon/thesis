#include "veb_ordering.h"

// TODO: optimize
static uint64_t get_height(uint64_t N) {
	// 0 => 0
	// 1 => 1
	// 2 => 2
	// 3 => 2
	// 4 => 3
	// 5 => 3
	// ...
	if (N == 0) return 0;
	for (uint64_t pow = 1, exp = 0; pow < N; pow *= 2, ++exp);
	return exp + 1;
}

static uint64_t floor_log2(uint64_t N) {
	if (N == 0) return 0;
	for (uint64_t pow = 1, exp = 0; pow < N; pow *= 2, ++exp);
	return exp + 1;
}

void order_as_veb(uint64_t height, uint64_t* output, uint64_t offset) {
	if (height == 0) {
		return;
	} else if (height == 1) {
		output[1] = offset + 0;
	} else {
		// makes heights of cut lines not vary with N
		uint64_t bottom_height = exp2(floor_log2(height - 1)),
				top_height = height - bottom_height;

		uint64_t i = 0;
		order_as_veb(top_height, output + i, offset + i);
		i += exp2(top_height);

		for (uint64_t j = 0; j < exp2(top_height); j++) {
			order_as_veb(bottom_height, output + i, offset + i);
			i += exp2(bottom_height);
		}
	}
}
