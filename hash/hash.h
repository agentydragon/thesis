#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

typedef struct hash_s hash;

typedef struct {
	int8_t (*init)(void**);
	void (*destroy)(void**);

	int8_t (*find)(void*, uint64_t key, uint64_t *value, bool *found);
	int8_t (*insert)(void*, uint64_t key, uint64_t value);
	int8_t (*delete)(void*, uint64_t key);
} hash_api;

int8_t hash_init(hash**, const hash_api* api);
void hash_destroy(hash**);

int8_t hash_find(hash*, uint64_t key, uint64_t *value, bool *found);
int8_t hash_insert(hash*, uint64_t key, uint64_t value);
int8_t hash_delete(hash*, uint64_t key);

#endif
