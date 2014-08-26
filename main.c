#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/errno.h>

#include "hash_array/hash_array.h"
#include "test_hash/test_hash.h"

#define error(fmt,...) fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__)

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	test_hash(&hash_array);

	// 4 GB
	// const size_t size = 1024 * 1024 * 1024;
	const size_t size = 4L * 1024L * 1024L * 1024L;
	void* memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (memory == MAP_FAILED) {
		error("mmap returned MAP_FAILED");
		switch (errno) {
			case EINVAL: error("EINVAL"); break;
			case EACCES: error("EACCES"); break;
			case ENOMEM: error("ENOMEM"); break;
			case ENODEV: error("ENODEV"); break;
			case ENOEXEC: error("ENOEXEC"); break;
		}
		return 1;
	}

	printf("%p\n", memory);

	for (int i = 0; i < 5; i++) {
		for (size_t i = 0; i < size; i++) {
			((char*) memory)[i] = i;
		}
	}

	if (munmap(memory, size) != 0) {
		error("munmap failed");
		return 1;
	}
}
