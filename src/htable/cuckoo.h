#ifndef HTABLE_CUCKOO_H
#define HTABLE_CUCKOO_H

#include "htable/hash.h"

#include <stdbool.h>

#define CUCKOO_EMPTY UINT64_MAX

// Cuckoo hash table with simple tabulation hashing.
// TODO: Use log(N)-independent hash function, which STH is not.
typedef struct {
	uint64_t *keys;
	uint64_t *values;
	sth hash;

	// Scratch space -- locations in other hash table we stole from.
	uint64_t *backptr;
} cuckoo_half;

typedef struct {
	uint64_t pair_count;
	uint64_t half_capacity;  // capacity of one half

	cuckoo_half left, right;
	rand_generator rand;
} htcuckoo;

void htcuckoo_init(htcuckoo* this, rand_generator rand);
void htcuckoo_destroy(htcuckoo* this);
int8_t htcuckoo_delete(htcuckoo* this, uint64_t key);
bool htcuckoo_find(htcuckoo* this, uint64_t key, uint64_t *value);
int8_t htcuckoo_insert(htcuckoo* this, uint64_t key, uint64_t value);

#endif
