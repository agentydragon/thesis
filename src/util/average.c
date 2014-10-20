#include "average.h"

#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

double histogram_average_i(int* array, int count) {
	int64_t sum = 0, div = 0;
	for (int i = 0; i < count; i++) {
		assert(array[i] >= 0);

		sum += array[i] * i;
		div += array[i];
	}
	assert(div > 0);
	return (double) sum / (double) div;
}
