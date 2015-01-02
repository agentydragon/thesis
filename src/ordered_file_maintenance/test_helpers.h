#ifndef ORDERED_FILE_MAINTENANCE_TEST_HELPERS_H
#define ORDERED_FILE_MAINTENANCE_TEST_HELPERS_H

#define COUNTOF(x) (sizeof(x) / sizeof(*(x)))
#define NIL 0xDEADDEADDEADDEAD
#define UNDEF 0xDEADBEEF

#include <stdint.h>
struct ordered_file;

void _set_key(struct ordered_file* file, uint64_t i, uint64_t key);
void destroy_file(struct ordered_file file);

void _assert_key(const struct ordered_file* file, uint64_t i, uint64_t expected);

uint64_t value_for_key(uint64_t key);

#define make_file(_file,_block_size,...) do { \
	const uint64_t _values[] = { __VA_ARGS__ }; \
	const uint64_t _count = COUNTOF(_values); \
	struct ordered_file* __file = (_file); \
	__file->occupied = calloc(_count, sizeof(bool)); \
	__file->items = calloc(_count, sizeof(ordered_file_item)); \
	__file->parameters.capacity = _count; \
	__file->parameters.block_size = _block_size; \
	for (uint64_t i = 0; i < _count; i++) { \
		_set_key(__file, i, _values[i]); \
	} \
} while (0)

#endif
