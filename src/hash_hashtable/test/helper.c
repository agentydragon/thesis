#include "hash_hashtable/test/helper.h"

#include "log/log.h"

uint64_t AMEN = 0xFFFFFFFFFFFFFFFF;

uint64_t hash_mock(void* _pairs, uint64_t key) {
	uint64_t* key_ptr = _pairs;

	do {
		if (*key_ptr == key) return *(key_ptr + 1);
		if (*key_ptr == AMEN) {
			log_fatal("hash_mock passed undefined key %ld\n", key);
			return -1;
		}
		key_ptr += 2;
	} while (true);
}
