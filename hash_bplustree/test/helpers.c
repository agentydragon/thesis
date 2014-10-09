#include "helpers.h"
#include "../private/data.h"

#include <inttypes.h>

#include "../../log/log.h"

void assert_key(const node* const target, uint64_t index, uint64_t expected) {
	if (target->keys[index] != expected) {
		log_fatal("node has key %" PRIu64 " equal to %" PRIu64 ", "
				"should be %" PRIu64,
				index, target->keys[index], expected);
	}
}

void assert_value(const node* const target, uint64_t index, uint64_t expected) {
	if (target->values[index] != expected) {
		log_fatal("node has value %" PRIu64 " equal to %" PRIu64 ", "
				"should be %" PRIu64,
				index, target->values[index], expected);
	}
}

void assert_pointer(const node* const target, uint64_t index,
		const void* const expected) {
	if (target->pointers[index] != expected) {
		log_fatal("node has pointer %" PRIu64 " equal to %p, "
				"should be %p",
				index, target->pointers[index], expected);
	}
}

void assert_n_keys(const node* const target, uint64_t expected) {
	if (target->keys_count != expected) { \
		log_fatal("node has %" PRIu64 " keys, should have %d",
				expected, target->keys_count);
	}
}
