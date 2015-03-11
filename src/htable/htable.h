#ifndef HTABLE_HTABLE_H
#define HTABLE_HTABLE_H

#include <stdint.h>
#include <stdbool.h>

const uint32_t HTABLE_KEYS_WITH_HASH_MAX;

// je to velke 56 bytu. keys_with_hash muze byt jenom uint32_t
// a je tam moc prazdneho prostoru :(
struct htable_block {
	uint64_t keys[3];
	uint64_t values[3];
	bool occupied[3];
	uint32_t keys_with_hash;

	uint8_t _unused[8]; // padded to 64 bytes
};

typedef struct {
	struct htable_block* blocks;
	uint64_t blocks_size;
	uint64_t pair_count;

	uint64_t (*hash_fn_override)(void* opaque, uint64_t key);
	void* hash_fn_override_opaque;
} htable;

int8_t htable_delete(htable* this, uint64_t key);
int8_t htable_find(void* _this, uint64_t key, uint64_t *value, bool *found);
int8_t htable_insert_internal(htable* this, uint64_t key, uint64_t value);
void htable_dump(void* this);

#endif
