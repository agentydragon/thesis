#include "cobt/pma_test.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log/log.h"
#include "math/math.h"
#include "cobt/pma.h"

void test_cobt_pma(void) {
	pma file;
	memset(&file, 0, sizeof(file));
	pma_init(&file);

	// Fills file with (1000 => 0), (999 => 500), (998 => 1000), ...
	for (uint64_t i = 0; i < 1000; i++) {
		void* mock_value = (void*) (i * 500);
		pma_insert_before(&file, (1000 - i), mock_value, file.capacity);
	}
	uint64_t seen = 0;
	for (uint64_t i = 0; i < file.capacity; i++) {
		if (file.occupied[i]) {
			CHECK(file.keys[i] == (1000 - seen),
					"expected key %" PRIu64 ", found "
					"%" PRIu64 ".", seen, file.keys[i]);
			assert(file.values[i] == (void*) (seen * 500));
			++seen;
		}
	}
	assert(seen == 1000);

	pma_destroy(&file);
}
