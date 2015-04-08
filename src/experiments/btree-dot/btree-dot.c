#include <stdio.h>
#include <inttypes.h>

#include "btree/btree.h"
#include "dict/btree.h"
#include "dict/dict.h"
#include "log/log.h"
#include "util/count_of.h"

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	dict* tree;
	ASSERT(dict_init(&tree, &dict_btree, NULL) == 0);

	const uint64_t keys[] = {
		4, 8, 15, 16, 23, 42, 80, 154, 160, 230, 298, 351, 628,
		458, 723, 314, 159, 97
	};
	for (uint64_t i = 0; i < COUNT_OF(keys); ++i) {
		log_info("insert %" PRIu64 "=42", keys[i]);
		ASSERT(dict_insert(tree, keys[i], 42) == 0);
	}

	const char* PATH = "experiments/btree-dot/btree.dot";

	FILE* output = fopen(PATH, "w");
	CHECK(output, "failed to open output file %s", PATH);
	btree_dump_dot(dict_get_implementation(tree), output);
	fclose(output);

	log_info("B-tree dot written to: %s", PATH);

	dict_destroy(&tree);
	return 0;
}
