#include <stdio.h>
#include <stdlib.h>

#include "hash_array/hash_array.h"
#include "hash_hashtable/hash_hashtable.h"

#include "test_hash/test_hash.h"
#include "test_hash/test_hash_large.h"
#include "test_observation/test_observation.h"
#include "test_rand/test_rand.h"

#include "log/log.h"

#include "performance/random_read.h"

void run_unit_tests() {
	test_rand();
	test_hash(&hash_array);
	test_hash(&hash_hashtable);
	test_observation(); // Uses hash_array.
//	test_hash_large(&hash_array);
	test_hash_large(&hash_hashtable);
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	run_unit_tests();
}
