#include "veb_ordering.h"

void test_veb_ordering() {
	uint64_t output[256];
	order_as_veb(1, output, 0);
	assert(output[0] == 0);

	order_as_veb(2, output, 0);
	assert(output[0] == 0);
	assert(output[1] == 1);
	assert(output[2] == 2);

	order_as_veb(3, output, 0);
	assert(output[0] == 0);
	assert(output[1] == 1);
	assert(output[2] == 2);
}
