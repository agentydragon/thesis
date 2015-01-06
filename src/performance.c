#include <stdio.h>
#include <stdlib.h>

#include "hash_array/hash_array.h"
#include "hash_bplustree/hash_bplustree.h"
#include "hash_cobt/hash_cobt.h"
#include "hash_hashtable/hash_hashtable.h"

#include "log/log.h"

#include "performance/random_read.h"

int test_mmap();

void run_performance_tests() {
	time_random_reads(&hash_array, 1000, 10000);
	time_random_reads(&hash_array, 10000, 100000);

	//log_info("Hash table");
	//for (uint64_t size = 10000; size < 20 * 1024ULL * 1024ULL;
	//		size *= 2) {
	//	time_random_reads(&hash_hashtable, size, size);
	//}

	//log_info("B+ tree");
	//for (uint64_t size = 10000; size < 20 * 1024ULL * 1024ULL;
	//		size *= 2) {
	//	time_random_reads(&hash_bplustree, size, size);
	//}

	log_info("Cache-oblivious B-tree");
	for (uint64_t size = 10000; size < 20 * 1024ULL * 1024ULL;
			size *= 2) {
		time_random_reads(&hash_cobt, size, size);
	}
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	run_performance_tests();
}
