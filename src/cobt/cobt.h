#ifndef COBT_H
#define COBT_H

// Cache-oblivious B-tree

#include <stdint.h>
#include "ofm/ofm.h"

#define COB_INFINITY UINT64_MAX

struct {
	uint64_t total_reorganized_size;
} COB_COUNTERS;

// 1) ordered file maintenance over N keys
//
// 2) VEB static pointing to maintained ordered file.
//	keys are all in leaves, inner nodes contain maxima (gaps = -infty)
//
//	log_(B+1) N search: hledam pres leve deti
//
//	INSERT: 1) najit successora. vlozim polozku do ordered filu,
//			updatuju strom.
//			podobne delete.
//	O(log_(B+1) N + (log^2 N)/B) block reads.
//
// 3) TODO: pridat indirekci abych se zbavil O((log^2 N) / B)

typedef struct {
	// vEB-layout nodes
	// ordered file structure
	ofm file;
	// contains 2*(ordered file subranges)-1 nodes
	uint64_t* veb_minima;

	struct level_data* level_data;
} cob;

void cob_init(cob* this);
void cob_destroy(cob this);
int8_t cob_insert(cob* this, uint64_t key, uint64_t value);
int8_t cob_delete(cob* this, uint64_t key);
void cob_find(cob* this, uint64_t key, bool *found, uint64_t *value);
void cob_next_key(cob* this, uint64_t key,
		bool *next_key_exists, uint64_t *next_key);
void cob_previous_key(cob* this, uint64_t key,
		bool *previous_key_exists, uint64_t *previous_key);
void cob_check(cob* this);

#endif
