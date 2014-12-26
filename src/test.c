#include <stdio.h>
#include <stdlib.h>

#include "hash_array/hash_array.h"
#include "hash_hashtable/hash_hashtable.h"
#include "hash_bplustree/hash_bplustree.h"

#include "test_hash/test_hash.h"
#include "test_hash/test_hash_large.h"

#include "hash_hashtable/test/test.h"
#include "hash_bplustree/test/test.h"
#include "observation/test/test.h"
#include "rand/test/test.h"
#include "veb_layout/test.h"
#include "math/test.h"

#include "log/log.h"

#include "performance/random_read.h"

void run_unit_tests() {
	test_veb_layout();

	test_math();
	test_rand();

	test_hash_hashtable();
	test_hash_bplustree();

	test_hash(&hash_array);
	test_hash(&hash_hashtable);

	test_observation(); // Uses hash_array.
	test_hash_large(&hash_array, 1 << 10);
	test_hash_large(&hash_hashtable, 1 << 20);
	test_hash_large(&hash_bplustree, 1 << 20);
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	run_unit_tests();
}
