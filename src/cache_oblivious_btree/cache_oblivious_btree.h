#ifndef CACHE_OBLIVIOUS_BTREE_H
#define CACHE_OBLIVIOUS_BTREE_H

#include <stdint.h>
#include "../ordered_file_maintenance/ordered_file_maintenance.h"

// TODO: boundary value handling!
#define COB_INFINITY 0xFFFFFFFFDEADBEEF

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

struct cob {
	// vEB-layout nodes
	// ordered file structure
	struct ordered_file file;
	// contains 2*(ordered file subranges)-1 nodes
	uint64_t* veb_minima;
};

void cob_insert(struct cob* this, uint64_t key, uint64_t value);
void cob_delete(struct cob* this, uint64_t key);
void cob_find(const struct cob* this, uint64_t key,
		bool *found, uint64_t *value);
void cob_next_key(const struct cob* this, uint64_t key,
		bool *next_key_exists, uint64_t *next_key);
void cob_previous_key(const struct cob* this, uint64_t key,
		bool *previous_key_exists, uint64_t *previous_key);
uint64_t get_veb_height(const struct cob* this);

#endif
