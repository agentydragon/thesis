#include <stdio.h>
#include <stdlib.h>

#include "btree/test.h"
#include "cobt/ofm_test.h"
#include "cobt/tree_test.h"
#include "dict/array.h"
#include "dict/btree.h"
#include "dict/cobt.h"
#include "dict/hashtable.h"
#include "dict/splay.h"
#include "dict/test/blackbox.h"
#include "dict/test/large.h"
#include "dict/test/ordered_dict_blackbox.h"
#include "hashtable/test/test.h"
#include "log/log.h"
#include "math/test.h"
#include "observation/test.h"
#include "performance/random_read.h"
#include "rand/test.h"
#include "splay/test.h"
#include "veb_layout/test.h"

void run_unit_tests() {
	test_cobt_tree();

	test_btree();

	test_math();
	test_rand();
	test_veb_layout();
	test_ofm();
	test_observation(); // Uses hash_array.

	test_hashtable();
	test_splay_tree();

	test_dict_blackbox(&dict_array);
	test_dict_blackbox(&dict_btree);
	test_dict_blackbox(&dict_cobt);
	test_dict_blackbox(&dict_hashtable);
	test_dict_blackbox(&dict_splay);

	// Disabled pending implementation of COBT next/previous API
	//   test_ordered_dict_blackbox(&dict_cobt);
	test_ordered_dict_blackbox(&dict_splay);

	test_dict_large(&dict_array, 1 << 10);
	test_dict_large(&dict_btree, 1 << 20);
	// TODO: optimize dict_cobt for better performance
	test_dict_large(&dict_cobt, 1 << 20);
	test_dict_large(&dict_hashtable, 1 << 20);
	test_dict_large(&dict_splay, 1 << 20);
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;
	run_unit_tests();
}
