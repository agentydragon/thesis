#ifndef ORDERED_HASH_BLACKBOX_TEST
#define ORDERED_HASH_BLACKBOX_TEST

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	void (*init)(void** _this);
	void (*destroy)(void** _this);
	void (*insert)(void* _this, uint64_t key, uint64_t value);
	void (*remove)(void* _this, uint64_t key);
	void (*find)(void* _this, uint64_t key, bool *found, uint64_t *found_value);
	void (*next_key)(void* _this, uint64_t key, bool *found, uint64_t *next_key);
	void (*previous_key)(void* _this, uint64_t key, bool *found, uint64_t *previous_key);

	void (*check)(void* _this);
} ordered_hash_blackbox_spec;

void test_ordered_hash_blackbox(ordered_hash_blackbox_spec api);

#endif
