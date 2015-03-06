#ifndef DICT_H_INCLUDED
#define DICT_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#define DICT_RESERVED_KEY 0xFFFFFFFFFFFFFFFF

typedef struct dict_s dict;

typedef struct {
	int8_t (*init)(void**, void* args);
	void (*destroy)(void**);

	int8_t (*find)(void*, uint64_t key, uint64_t *value, bool *found);
	int8_t (*insert)(void*, uint64_t key, uint64_t value);
	int8_t (*delete)(void*, uint64_t key);

	const char* name;

	// Optional extension: ordered dictionary
	int8_t (*next)(void*, uint64_t key, uint64_t *next_key, bool *found);
	int8_t (*prev)(void*, uint64_t key, uint64_t *prev_key, bool *found);

	// Optional
	void (*dump)(void*);
	void (*check)(void*);
} dict_api;

int8_t dict_init(dict**, const dict_api* api, void* args);
void dict_destroy(dict**);

int8_t dict_find(dict*, uint64_t key, uint64_t *value, bool *found);
int8_t dict_insert(dict*, uint64_t key, uint64_t value);
int8_t dict_delete(dict*, uint64_t key);

int8_t dict_next(dict*, uint64_t key, uint64_t *next_key, bool *found);
int8_t dict_prev(dict*, uint64_t key, uint64_t *prev_key, bool *found);

void dict_dump(dict*);
void dict_check(dict*);

#endif
