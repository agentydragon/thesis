#ifndef DICT_H_INCLUDED
#define DICT_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#define DICT_RESERVED_KEY UINT64_MAX

typedef struct dict_s dict;

typedef struct {
	int8_t (*init)(void**, void* args);
	void (*destroy)(void**);

	void (*find)(void*, uint64_t key, uint64_t *value, bool *found);
	int8_t (*insert)(void*, uint64_t key, uint64_t value);
	int8_t (*delete)(void*, uint64_t key);

	const char* name;

	// Optional extension: ordered dictionary.
	// NULL if not implemented.
	void (*next)(void*, uint64_t key, uint64_t *next_key, bool *found);
	void (*prev)(void*, uint64_t key, uint64_t *prev_key, bool *found);

	// Optional
	void (*dump)(void*);
	void (*check)(void*);
} dict_api;

int8_t dict_init(dict**, const dict_api* api, void* args);
void dict_destroy(dict**);

void dict_find(dict*, uint64_t key, uint64_t *value, bool *found);
int8_t dict_insert(dict*, uint64_t key, uint64_t value);
int8_t dict_delete(dict*, uint64_t key);

void dict_next(dict*, uint64_t key, uint64_t *next_key, bool *found);
void dict_prev(dict*, uint64_t key, uint64_t *prev_key, bool *found);

void dict_dump(dict*);
void dict_check(dict*);

// Provides access to implementation pointer. Explicitly disallows modification.
const void* dict_get_implementation(dict*);
const dict_api* dict_get_api(dict*);

// Derived from dict_find.
// TODO: Some implementations may want to provide a custom implementation.
bool dict_contains(dict*, uint64_t key);

#endif
