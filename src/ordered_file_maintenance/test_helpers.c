#include "test_helpers.h"
#include "ordered_file_maintenance.h"
#include <assert.h>
#include <stdlib.h>

uint64_t value_for_key(uint64_t key) {
	return (key * 100) + 5;
}

void _set_key(struct ordered_file* file, uint64_t i, uint64_t key) {
	if (key == NIL) {
		file->occupied[i] = false;
		file->items[i].key = UNDEF;
	} else {
		file->occupied[i] = true;
		file->items[i].key = key;
	}
}

void _assert_key(const struct ordered_file* file, uint64_t i, uint64_t expected) {
	if (expected == NIL) {
		assert(!file->occupied[i]);
	} else {
		assert(file->occupied[i]);
		assert(file->items[i].key == expected);
		// TODO: also check value
	}
}

void destroy_file(struct ordered_file file) {
	free(file.occupied);
	free(file.items);
}
