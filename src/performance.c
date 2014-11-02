#include <stdio.h>
#include <stdlib.h>

#include "hash_array/hash_array.h"
#include "hash_hashtable/hash_hashtable.h"
#include "hash_bplustree/hash_bplustree.h"

#include "log/log.h"

#include "performance/random_read.h"

int test_mmap();

void run_performance_tests() {
	time_random_reads(&hash_array, 1000, 10000);

	time_random_reads(&hash_array, 10000, 100000);

	time_random_reads(&hash_hashtable, 10000, 200000);
	time_random_reads(&hash_hashtable, 20000, 200000);
	time_random_reads(&hash_hashtable, 50000, 200000);
	time_random_reads(&hash_hashtable, 100000, 200000);
	time_random_reads(&hash_hashtable, 200000, 200000);
	time_random_reads(&hash_hashtable, 1000000, 2000000);
	time_random_reads(&hash_hashtable, 2000000, 10000000);
	time_random_reads(&hash_hashtable, 5000000, 10000000);
	// time_random_reads(&hash_hashtable, 10000000, 20000000);
	// time_random_reads(&hash_hashtable, 20000000, 100000000);
	// time_random_reads(&hash_hashtable, 50000000, 100000000);

	time_random_reads(&hash_bplustree, 10000, 200000);
	time_random_reads(&hash_bplustree, 20000, 200000);
	time_random_reads(&hash_bplustree, 50000, 200000);
	time_random_reads(&hash_bplustree, 100000, 200000);
	time_random_reads(&hash_bplustree, 200000, 200000);
	time_random_reads(&hash_bplustree, 1000000, 2000000);
	time_random_reads(&hash_bplustree, 2000000, 10000000);
	time_random_reads(&hash_bplustree, 5000000, 10000000);
	time_random_reads(&hash_bplustree, 10000000, 10000000);
	time_random_reads(&hash_bplustree, 20000000, 10000000);
	time_random_reads(&hash_bplustree, 50000000, 10000000);

//	if (test_mmap()) return 1;
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	run_performance_tests();
}
