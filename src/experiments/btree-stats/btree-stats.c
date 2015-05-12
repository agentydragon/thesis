#include <inttypes.h>
#include <math.h>
#include <stdio.h>

#include "btree/btree.h"
#include "dict/btree.h"
#include "dict/dict.h"
#include "dict/test/toycrypt.h"
#include "log/log.h"
#include "util/count_of.h"

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	dict* tree;
	dict_init(&tree, &dict_btree);

	const uint64_t N = 1000 * 1000;

	for (uint64_t i = 0; i < N; ++i) {
		const uint64_t key = toycrypt(i, 42);
		const uint64_t value = toycrypt(i, 999);
		ASSERT(dict_insert(tree, key, value));
	}

	log_info("inserted %" PRIu64 " keys. internal n keys histogram:", N);
	btree_stats stats = btree_collect_stats(dict_get_implementation(tree));
	for (uint8_t i = 0; i < 100; ++i) {
		if (stats.internal_n_keys_histogram[i] > 0) {
			log_info("[%" PRIu8 "] = %" PRIu64, i, stats.internal_n_keys_histogram[i]);
		}
	}

	const double avg_kvp_depth = ((double) stats.total_kvp_path_length) / stats.total_kvps;
	log_info("average key-value pair depth: %.2lf", avg_kvp_depth);
	log_info("effective branching factor: %.2lf", pow(N, 1.0 / avg_kvp_depth));

	dict_destroy(&tree);
	return 0;
}
