#include <stdio.h>
#include <stdlib.h>

#include "dict/array.h"
#include "dict/btree.h"
#include "dict/cobt.h"
#include "dict/hashtable.h"
#include "dict/splay.h"
#include "log/log.h"
#include "performance/random_read.h"

int test_mmap();

void run_performance_tests() {
	time_random_reads(&dict_array, 1000, 10000);
	time_random_reads(&dict_array, 10000, 100000);

	log_info("Hash table");
	for (uint64_t size = 10000; size < 20 * 1024ULL * 1024ULL;
			size *= 2) {
		time_random_reads(&dict_hashtable, size, size);
	}

	log_info("B+ tree");
	for (uint64_t size = 10000; size < 20 * 1024ULL * 1024ULL;
			size *= 2) {
		time_random_reads(&dict_btree, size, size);
	}

	log_info("Splay tree");
	for (uint64_t size = 10000; size < 20 * 1024ULL * 1024ULL;
			size *= 2) {
		time_random_reads(&dict_splay, size, size);
	}

	log_info("COB tree");
	for (uint64_t size = 10000; size < 20 * 1024ULL * 1024ULL;
			size *= 2) {
		time_random_reads(&dict_cobt, size, size);
	}
}

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	run_performance_tests();
}
