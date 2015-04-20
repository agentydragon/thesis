#ifndef DICT_H_INCLUDED
#define DICT_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#include "util/unused.h"

#define DICT_RESERVED_KEY UINT64_MAX

typedef struct dict_s dict;

typedef struct {
	int8_t (*init)(void**, void* args);
	void (*destroy)(void**);

	bool (*find)(void*, uint64_t key, uint64_t *value);
	int8_t (*insert)(void*, uint64_t key, uint64_t value);
	int8_t (*delete)(void*, uint64_t key);

	const char* name;

	// Optional extension: ordered dictionary.
	// NULL if not implemented.
	bool (*next)(void*, uint64_t key, uint64_t *next_key);
	bool (*prev)(void*, uint64_t key, uint64_t *prev_key);

	// Optional
	void (*dump)(void*);
	void (*check)(void*);
} dict_api;

int8_t dict_init(dict**, const dict_api* api, void* args);
void dict_destroy(dict**);

bool MUST_USE_RESULT dict_find(dict*, uint64_t key, uint64_t *value);
int8_t MUST_USE_RESULT dict_insert(dict*, uint64_t key, uint64_t value);
int8_t MUST_USE_RESULT dict_delete(dict*, uint64_t key);

// Optional extension: ordered dictionary.
bool dict_api_allows_order_queries(const dict_api*);
bool dict_allows_order_queries(const dict*);
bool MUST_USE_RESULT dict_next(dict*, uint64_t key, uint64_t *next_key);
bool MUST_USE_RESULT dict_prev(dict*, uint64_t key, uint64_t *prev_key);

void dict_dump(dict*);
void dict_check(dict*);

// Provides access to implementation pointer. Explicitly disallows modification.
const void* dict_get_implementation(dict*);
const dict_api* dict_get_api(dict*);

// Derived from dict_find.
// TODO: Some implementations may want to provide a custom implementation.
bool dict_contains(dict*, uint64_t key);

#endif
