#include "cobt/cobt.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "math/math.h"
#include "veb_layout/veb_layout.h"

#include "log/log.h"

#define EMPTY UINT64_MAX

typedef struct {
	uint64_t key;
	uint64_t value;
} piece_item;

static void dump_piece(char* buffer, cob* this, uint64_t index) {
	piece_item* piece = ofm_get_value(&this->file, index);
	uint8_t z = 0;
	z += sprintf(buffer + z, "[%" PRIu64 " %s key=%" PRIu64 "]: ",
			index,
			this->file.occupied[index] ? "occupied" : "unoccupied",
			this->file.keys[index]);
	for (uint8_t i = 0; i < this->piece; i++) {
		if (piece[i].key == EMPTY) {
			z += sprintf(buffer + z, "(empty) ");
		} else {
			z += sprintf(buffer + z, "%" PRIu64 "=%" PRIu64 " ",
					piece[i].key, piece[i].value);
		}
	}
}

static void internal_check(cob* this) {
	for (uint64_t i = 0; i < this->file.capacity; i++) {
		if (this->file.occupied[i]) {
			assert(this->file.keys[i] != EMPTY);

			piece_item* piece = ofm_get_value(&this->file, i);
			if (this->file.keys[i] != piece[0].key) {
				char buffer[1024];
				dump_piece(buffer, this, i);
				log_fatal("piece not correctly keyed: %s", buffer);
			}

			for (uint8_t j = 0; j < this->piece - 1; j++) {
				if (!(piece[j].key <= piece[j + 1].key)) {
					char buffer[1024];
					dump_piece(buffer, this, i);
					log_fatal("failure in piece: %s", buffer);
				}
			}
		}
	}
	// Possibly add more checks later.
}

static void dump_all(cob* this) {
	for (uint64_t i = 0; i < this->file.capacity; i++) {
		char buffer[1024];
		dump_piece(buffer, this, i);
		log_info("%s", buffer);
	}
	cobt_tree_dump(&this->tree);
}

static uint8_t piece_size(cob* this, piece_item* piece) {
	uint8_t n = 0;
	for (uint8_t i = 0; i < this->piece; i++) {
		if (piece[i].key != EMPTY) {
			++n;
		}
	}
	return n;
}

static void enforce_piece_policy(cob* this, uint64_t new_size);

static void refresh_piece_key(cob* this, uint64_t index) {
	// char buffer[1024];
	// dump_piece(buffer, this, index);
	// log_info("refresh_piece_key(%s)", buffer);
	piece_item* piece = ofm_get_value(&this->file, index);
	if (this->file.keys[index] == piece[0].key) {
		// log_info("no need to update piece key %" PRIu64, index);
	} else {
		// log_info("updating piece %" PRIu64 " key from %" PRIu64 " to %" PRIu64,
		// 		index, this->file.keys[index], piece[0].key);
		// char buffer[1024];
		// dump_piece(buffer, this, index);
		// log_info("piece: %s", buffer);
		this->file.keys[index] = piece[0].key;
		cobt_tree_refresh(&this->tree, (cobt_tree_range) {
			.begin = index,
			.end = index + 1
		});
	}
}

static void fix_range(cob* this, ofm_range range_to_fix) {
	cobt_tree_refresh(&this->tree, (cobt_tree_range) {
		.begin = range_to_fix.begin,
		.end = range_to_fix.begin + range_to_fix.size
	});
}

static void entirely_reset_veb(cob* this) {
	cobt_tree_destroy(&this->tree);
	cobt_tree_init(&this->tree, this->file.keys, this->file.occupied,
			this->file.capacity);
}

static void validate_key(uint64_t key) {
	CHECK(key != COB_INFINITY, "Trying to operate on key=COB_INFINITY");
}

static int compare_piece_items(const void* _a, const void* _b) {
	const piece_item* a = _a, *b = _b;
	if (a->key < b->key) {
		return -1;
	} else if (a->key == b->key) {
		return 0;
	} else {
		return 1;
	}
}

static void sort_piece(cob* this, piece_item* piece) {
	qsort(piece, this->piece, sizeof(piece_item), compare_piece_items);
}

static void sort_piece_c(uint64_t size, piece_item* piece) {
	qsort(piece, size, sizeof(piece_item), compare_piece_items);
}

static bool piece_full(cob* this, piece_item* piece) {
	for (uint8_t i = 0; i < this->piece; i++) {
		if (piece[i].key == EMPTY) return false;
	}
	return true;
}

static bool delete_from_piece(cob* this, piece_item* piece, uint64_t key) {
	for (uint8_t i = 0; i < this->piece; i++) {
		if (piece[i].key == key) {
			piece[i].key = EMPTY;
			piece[i].value = EMPTY;
			sort_piece(this, piece);
			return true;
		}
	}
	return false;
}

// TODO: I could merge the key and the first piece item to save an uint64_t
// for every block.
static void insert_into_piece(cob* this, uint64_t index,
		uint64_t key, uint64_t value) {
	piece_item* piece = ofm_get_value(&this->file, index);
	for (uint8_t i = 0; i < this->piece; i++) {
		if (piece[i].key == EMPTY) {
			// OK, we still have some space here.
			piece[i].key = key;
			piece[i].value = value;
			sort_piece(this, piece);

			// char buffer[1024];
			// dump_piece(buffer, this, index);
			// log_info("piece after insertion: %s", buffer);

			return;
		}
	}
	log_fatal("inserting into full piece");
}

static void clear_piece(cob* this, piece_item* piece) {
	for (uint8_t i = 0; i < this->piece; i++) {
		piece[i].key = EMPTY;
		piece[i].value = EMPTY;
	}
}

static void insert_first_piece(cob* this, uint64_t key, piece_item* new_piece) {
	assert(this->size == 0);
	// TODO: Factor out the "save-change-check-fix" snippet.
	const uint64_t prior_capacity = this->file.capacity;
	ofm_range reorg_range = ofm_insert_before(&this->file, key, new_piece,
			this->file.capacity, NULL);
	if (this->file.capacity == prior_capacity) {
		fix_range(this, reorg_range);
	} else {
		entirely_reset_veb(this);
	}
}

static void split_piece(cob* this, uint64_t index) {
	piece_item* old_piece = ofm_get_value(&this->file, index);

	if (piece_size(this, old_piece) < (this->piece * 3) / 4) {
		// The piece is not too full yet.
		return;
	}

	// log_info("splitting piece %" PRIu64, index);

	piece_item* new_piece = alloca(this->piece * sizeof(piece_item));
	clear_piece(this, new_piece);
	const uint8_t start = this->piece / 2;
	for (uint8_t i = start; i < this->piece; i++) {
		new_piece[i - start].key = old_piece[i].key;
		new_piece[i - start].value = old_piece[i].value;

		old_piece[i].key = EMPTY;
		old_piece[i].value = EMPTY;
	}

	uint64_t after;
	for (after = index + 1; after < this->file.capacity; after++) {
		if (this->file.occupied[after]) {
			break;
		}
	}
	const uint64_t prior_capacity = this->file.capacity;
	ofm_range reorg_range =  ofm_insert_before(&this->file,
			new_piece[0].key, new_piece, after, NULL);
	if (this->file.capacity == prior_capacity) {
		fix_range(this, reorg_range);
	} else {
		entirely_reset_veb(this);
	}

	// log_info("After split:");
	// dump_all(this);
}

int8_t cob_insert(cob* this, uint64_t key, uint64_t value) {
	validate_key(key);
	enforce_piece_policy(this, this->size + 1);

	// dump_all(this);

	const uint64_t index = cobt_tree_find_le(&this->tree, key);

	piece_item* piece = ofm_get_value(&this->file, index);
	// char buffer[1024] = { 0 };
	// dump_piece(buffer, this, index);
	// log_info("key %" PRIu64 " to insert in %s piece %" PRIu64 " [[ %s]]",
	// 		key,
	// 		this->file.occupied[index] ? "OCCUPIED" : "blank",
	// 		index, buffer);
	if (this->file.occupied[index]) {
		for (uint8_t i = 0; i < this->piece; i++) {
			if (piece[i].key == key) {
				// Duplicate key.
				goto duplicate_key;
			}
		}
		assert(!piece_full(this, piece));
		insert_into_piece(this, index, key, value);
		refresh_piece_key(this, index);

		// We may need a split, which can invalidate our
		// old pointer.
		split_piece(this, index);
	} else {
		assert(this->size == 0);
		// Special case: create new piece.
		piece_item* new_piece = alloca(this->piece * sizeof(piece_item));
		new_piece[0].key = key;
		new_piece[0].value = value;
		for (uint8_t i = 1; i < this->piece; i++) {
			new_piece[i].key = EMPTY;
			new_piece[i].value = EMPTY;
		}
		insert_first_piece(this, key, new_piece);
	}
	++this->size;

	// log_info("cob_insert(%" PRIu64 "=%" PRIu64 ") done", key, value);
	// internal_check(this);
	return 0;

duplicate_key:
	// log_info("cob_insert(%" PRIu64 "=%" PRIu64 "): duplicate", key, value);
	// internal_check(this);
	return 1;
}

//	COB_COUNTERS.total_reorganized_size += reorg_range.size;

static void merge_pieces(cob* this, uint64_t left, uint64_t right) {
	piece_item* l = ofm_get_value(&this->file, left),
			*r = ofm_get_value(&this->file, right);
	const uint8_t left_size = piece_size(this, l),
		      right_size = piece_size(this, r);

	// dump_all(this);

	// char buffer[1024];
	// dump_piece(buffer, this, left);
	// log_info("before: left: %s", buffer);
	// dump_piece(buffer, this, right);
	// log_info("before: right: %s", buffer);

	if (left_size + right_size < (this->piece * 3) / 4) {
		// Merge to one node.
		for (uint8_t i = 0; i < right_size; i++) {
			l[i + left_size].key = r[i].key;
			l[i + left_size].value = r[i].value;

			r[i].key = EMPTY;
			r[i].value = EMPTY;
		}
		sort_piece(this, l);
		sort_piece(this, r);

		// dump_piece(buffer, this, left);
		// log_info("after: left: %s", buffer);
		// dump_piece(buffer, this, right);
		// log_info("after: right: %s", buffer);

		refresh_piece_key(this, left);
		refresh_piece_key(this, right);

		const uint64_t prior_capacity = this->file.capacity;
		ofm_range reorg_range = ofm_delete(&this->file,
				right, NULL);
		// log_info("reorg range: %" PRIu64 "+%" PRIu64,
		// 		reorg_range.begin, reorg_range.size);
		if (this->file.capacity == prior_capacity) {
			fix_range(this, reorg_range);
		} else {
			entirely_reset_veb(this);
		}
		// dump_all(this);
	} else {
		// Left eats everything, right gets killed off.
		// TODO: fuj
		piece_item* items = alloca((left_size + right_size) * sizeof(piece_item));
		for (uint8_t i = 0; i < left_size; i++) {
			items[i].key = l[i].key;
			items[i].value = l[i].value;
		}
		for (uint8_t i = 0; i < right_size; i++) {
			items[left_size + i].key = r[i].key;
			items[left_size + i].value = r[i].value;
		}

		clear_piece(this, l);
		clear_piece(this, r);

		const uint8_t new_l = (left_size + right_size) / 2,
			      new_r = (left_size + right_size) - new_l;
		// log_info("new_l=%" PRIu64 " new_r=%" PRIu64);
		for (uint8_t i = 0; i < new_l; i++) {
			l[i].key = items[i].key;
			l[i].value = items[i].value;
		}
		for (uint8_t i = 0; i < new_r; i++) {
			r[i].key = items[new_l + i].key;
			r[i].value = items[new_l + i].value;
		}

		sort_piece(this, l);
		sort_piece(this, r);

		refresh_piece_key(this, left);
		refresh_piece_key(this, right);
	}
}

static void merge_piece(cob* this, uint64_t piece_index) {
	piece_item* piece = ofm_get_value(&this->file, piece_index);
	if (piece_size(this, piece) >= this->piece / 4) {
		// This piece is still OK.
		return;
	}

	// Try to find a right neighbour.
	uint64_t right_neighbour_index;
	for (right_neighbour_index = piece_index + 1;
			right_neighbour_index < this->file.capacity;
			right_neighbour_index++) {
		if (this->file.occupied[right_neighbour_index]) {
			// log_info("will merge piece %" PRIu64 " "
			// 		"with right: %" PRIu64,
			// 		piece_index, right_neighbour_index);
			merge_pieces(this, piece_index, right_neighbour_index);
			return;
		}
	}

	uint64_t left_neighbour_index;
	for (uint64_t i = 0; i < piece_index; i++) {
		left_neighbour_index = piece_index - 1 - i;
		if (this->file.occupied[left_neighbour_index]) {
			// log_info("will merge piece %" PRIu64 " "
			// 		"with left: %" PRIu64,
			// 		piece_index, left_neighbour_index);
			merge_pieces(this, left_neighbour_index, piece_index);
			return;
		}
	}
	// Got nothing to merge with. This is not a problem, since it's
	// a singeton block.
	// We may need to mark it unoccupied, through.
	// log_info("LAST CASE...");
	if (piece_size(this, piece) == 0) {
		const uint64_t prior_capacity = this->file.capacity;
		ofm_range reorg_range =  ofm_delete(&this->file,
				piece_index, NULL);
		if (this->file.capacity == prior_capacity) {
			fix_range(this, reorg_range);
		} else {
			entirely_reset_veb(this);
		}
	}
}

int8_t cob_delete(cob* this, uint64_t key) {
	// log_info("cob_delete(%" PRIu64 ") begin", key);
	validate_key(key);
	if (this->size >= 1) {
		enforce_piece_policy(this, this->size - 1);
	}

	const uint64_t index = cobt_tree_find_le(&this->tree, key);
	if (!this->file.occupied[index]) {
		// Deleting nonexistant key.
		goto nonexistant_key;
	}

	piece_item* piece = ofm_get_value(&this->file, index);
	if (!delete_from_piece(this, piece, key)) {
		// Deleting nonexistant key.
		goto nonexistant_key;
	}

	// log_info("updating piece key to reflect deletion of %" PRIu64, key);
	refresh_piece_key(this, index);

	// log_info("merging piece with neighbours");
	merge_piece(this, index);
	--this->size;
	// log_info("cob_delete(%" PRIu64 ") done", key);
	// dump_all(this);
	// internal_check(this);
	return 0;

nonexistant_key:
	// log_info("cob_delete(%" PRIu64 ") deleting nonexistant key", key);
	// internal_check(this);
	return 1;
}

void cob_find(cob* this, uint64_t key, bool *found, uint64_t *value) {
	validate_key(key);
	const uint64_t index = cobt_tree_find_le(&this->tree, key);
	if (this->file.occupied[index]) {
		// Look up the key in this piece.
		piece_item* piece = ofm_get_value(&this->file, index);
		for (uint8_t i = 0; i < this->piece; i++) {
			if (piece[i].key == key) {
				if (value != NULL) {
					*value = piece[i].value;
				}
				// log_info("cob_find(%" PRIu64 ") found %" PRIu64,
				// 		key, piece[i].value);
				*found = true;
				return;
			}
		}
	}
	// log_info("cob_find(%" PRIu64 "): no such key", key);
	*found = false;
}

void cob_next_key(cob* this, uint64_t key,
		bool *next_key_exists, uint64_t* next_key) {
	(void) this; (void) key; (void) next_key_exists; (void) next_key;
	log_fatal("cob_next_key not implemented yet");
}

void cob_previous_key(cob* this, uint64_t key,
		bool *previous_key_exists, uint64_t* previous_key) {
	(void) this; (void) key;
	(void) previous_key_exists; (void) previous_key;
	log_fatal("cob_previous_key not implemented yet");
}

static void rebuild_file(cob* this, uint8_t new_piece, ofm* new_file) {
	// log_info("rebuilding");
	ofm_init(new_file, new_piece * sizeof(piece_item));

	const uint8_t preferred_piece = new_piece / 2;
	uint8_t got_now = 0;
	piece_item* tmp = alloca(new_piece * sizeof(piece_item));
	for (uint64_t i = 0; i < new_piece; i++) {
		tmp[i].key = EMPTY;
		tmp[i].value = EMPTY;
	}

	for (uint64_t i = 0; i < this->file.capacity; i++) {
		if (!this->file.occupied[i]) continue;
		piece_item* this_piece = ofm_get_value(&this->file, i);
		for (uint8_t j = 0; j < this->piece; j++) {
			if (this_piece[j].key == EMPTY) {
				continue;
			}
			assert(got_now < preferred_piece);
			tmp[got_now].key = this_piece[j].key;
			tmp[got_now].value = this_piece[j].value;
			++got_now;

			// log_info("push %" PRIu64 "=%" PRIu64,
			// 		this_piece[j].key, this_piece[j].value);

			if (got_now == preferred_piece) {
				// log_info("flush");
				sort_piece_c(preferred_piece, tmp);
				ofm_insert_before(new_file, tmp[0].key, tmp,
						new_file->capacity, NULL);
				got_now = 0;
				for (uint8_t z = 0; z < new_piece; z++) {
					tmp[z].key = EMPTY;
					tmp[z].value = EMPTY;
				}
			}
		}
	}

	if (got_now > 0) {
		// log_info("flush");
		sort_piece_c(preferred_piece, tmp);
		ofm_insert_before(new_file, tmp[0].key, tmp,
				new_file->capacity, NULL);
		got_now = 0;
	}
	// log_info("rebuilt file to piece %" PRIu64, new_piece);
}

static void enforce_piece_policy(cob* this, uint64_t new_size) {
	const uint8_t log = ceil_log2(new_size);
	uint8_t new_piece = this->piece;
	while (log >= new_piece + 4) {
		new_piece += 4;
	}
	while (new_piece > 4 && log <= new_piece - 4) {
		new_piece -= 4;
	}
	if (this->piece != new_piece) {
		// log_info("piece resizing from %" PRIu8 " to %" PRIu8,
		// 		this->piece, new_piece);
		// Here we should probably start inserting items to a new
		// file left to right, and then we should rebuild a new
		// vEB tree on top of that.
		ofm file;
		rebuild_file(this, new_piece, &file);

		ofm_destroy(this->file);
		this->file = file;

		entirely_reset_veb(this);
		this->piece = new_piece;
	}
}

void cob_init(cob* this) {
	this->size = 0;
	this->piece = 4;  // Initial piece size: 4
	ofm_init(&this->file, this->piece * sizeof(piece_item));
	cobt_tree_init(&this->tree, this->file.keys, this->file.occupied,
			this->file.capacity);
}

void cob_destroy(cob* this) {
	ofm_destroy(this->file);
	cobt_tree_destroy(&this->tree);
}
