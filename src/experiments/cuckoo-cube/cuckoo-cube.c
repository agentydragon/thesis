#include <inttypes.h>
#include <stdio.h>

#include "dict/dict.h"
#include "dict/htcuckoo.h"
#include "htable/cuckoo.h"
#include "log/log.h"

FILE* output;

void try_with(uint64_t side) {
	dict* htable;
	CUCKOO_COUNTERS.full_rehashes = 0;
	ASSERT(!dict_init(&htable, &dict_htcuckoo, NULL));
	for (uint64_t i = 0; i < side; ++i) {
		for (uint64_t j = 0; j < side; ++j) {
			for (uint64_t k = 0; k < side; ++k) {
				const uint64_t key =
					(i << 32ULL) | (j << 16ULL) | k;
				ASSERT(!dict_insert(htable, key, 42));
			}
		}
	}
	log_info("side=%5" PRIu64 " size=%" PRIu64 ": "
			"took %5" PRIu64 " full rehashes",
			side, side * side * side,
			CUCKOO_COUNTERS.full_rehashes);
	fprintf(output, "%" PRIu64 "\t%" PRIu64 "\n",
			side, CUCKOO_COUNTERS.full_rehashes);
	fflush(output);
	dict_destroy(&htable);
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;
	output = fopen("experiments/cuckoo-cube/output.csv", "w");
	ASSERT(output);
	for (uint64_t side = 10; side < 1000; ++side) {
		try_with(side);
	}
	fclose(output);
	return 0;
}
