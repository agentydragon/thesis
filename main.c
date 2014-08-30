#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/errno.h>

#include "hash_array/hash_array.h"
#include "hash_hashtable/hash_hashtable.h"

#include "test_hash/test_hash.h"
#include "test_observation/test_observation.h"
#include "test_hash/test_hash_large.h"

#include "log/log.h"

int test_mmap();

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	test_hash(&hash_array);
	test_hash(&hash_hashtable);

	test_observation(); // Uses hash_array.

//	test_hash_large(&hash_array);
	test_hash_large(&hash_hashtable);

//	if (test_mmap()) return 1;
}


int test_mmap() {
	// 4 GB
	// const size_t size = 1024 * 1024 * 1024;
	const size_t size = 4L * 1024L * 1024L * 1024L;
	void* memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (memory == MAP_FAILED) {
		log_error("mmap returned MAP_FAILED");
		switch (errno) {
			case EINVAL: log_error("EINVAL"); break;
			case EACCES: log_error("EACCES"); break;
			case ENOMEM: log_error("ENOMEM"); break;
			case ENODEV: log_error("ENODEV"); break;
			case ENOEXEC: log_error("ENOEXEC"); break;
		}
		return 1;
	}

	log_info("mmap'd ptr %p", memory);

	for (int i = 0; i < 5; i++) {
		for (size_t i = 0; i < size; i++) {
			((char*) memory)[i] = i;
		}
	}

	if (munmap(memory, size) != 0) {
		log_error("munmap failed");
		return 1;
	}
	return 0;
}
