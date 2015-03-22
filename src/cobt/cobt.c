#include "cobt/cobt.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "log/log.h"
#include "math/math.h"
#include "util/unused.h"
#include "veb_layout/veb_layout.h"

#define EMPTY UINT64_MAX

typedef struct {
	uint64_t key;
	uint64_t value;
} piece_item;

static void fix_range(cob* this, pma_range range_to_fix) {
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

static void insert_piece_before(cob* this, piece_item* piece,
		uint64_t insert_before) {
	const uint64_t prior_capacity = this->file.capacity;
	pma_range reorg_range =  pma_insert_before(&this->file,
			piece[0].key, piece, insert_before);
	if (this->file.capacity == prior_capacity) {
		fix_range(this, reorg_range);
	} else {
		entirely_reset_veb(this);
	}
}

static piece_item* get_piece_start(cob* this, uint64_t index) {
	return pma_get_value(&this->file, index);
}

static void delete_piece(cob* this, uint64_t index) {
	const uint64_t prior_capacity = this->file.capacity;
	free(get_piece_start(this, index));
	pma_range reorg_range = pma_delete(&this->file, index);
	if (this->file.capacity == prior_capacity) {
		fix_range(this, reorg_range);
	} else {
		entirely_reset_veb(this);
	}
}

static void UNUSED describe_piece(char* buffer, cob* this, uint64_t index) {
	piece_item* piece = get_piece_start(this, index);
	uint8_t z = 0;
	z += sprintf(buffer + z, "[%" PRIu64 " =%p %s key=%" PRIu64 "]: ",
			index, piece,
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

static void UNUSED internal_check(cob* this) {
	for (uint64_t i = 0; i < this->file.capacity; i++) {
		if (this->file.occupied[i]) {
			char description[1024];
			describe_piece(description, this, i);
			assert(this->file.keys[i] != EMPTY);

			piece_item* piece = get_piece_start(this, i);
			CHECK(this->file.keys[i] == piece[0].key,
				"not correctly keyed: %s", description);

			for (uint8_t j = 0; j < this->piece - 1; j++) {
				CHECK(piece[j].key <= piece[j + 1].key,
					"bad order: %s", description);
			}
		}
	}
	// Possibly add more checks later.
}

static void UNUSED dump_all(cob* this) {
	for (uint64_t i = 0; i < this->file.capacity; i++) {
		char description[1024];
		describe_piece(description, this, i);
		log_info("%s", description);
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

static pma rebuild_file(cob* this, uint64_t new_size, uint8_t new_piece);

static void enforce_piece_policy(cob* this, uint64_t new_size) {
	const uint8_t log = new_size == 0 ? 1 : ceil_log2(new_size);
	uint8_t new_piece = this->piece;
	while (log >= new_piece + 4) {
		new_piece += 4;
	}
	while (new_piece > 4 && log <= new_piece - 4) {
		new_piece -= 4;
	}
	if (this->piece != new_piece) {
		log_info("%" PRIu64 " repiecing: %" PRIu8 " -> %" PRIu8,
				new_size, this->piece, new_piece);
		const pma new_file = rebuild_file(this, new_size, new_piece);
		pma_destroy(&this->file);
		this->file = new_file;

		entirely_reset_veb(this);
		this->piece = new_piece;
	}
}

static void refresh_piece_key(cob* this, uint64_t index) {
	// char description[1024];
	// describe_piece(description, this, index);
	// log_info("refresh_piece_key(%s)", buffer);
	piece_item* piece = get_piece_start(this, index);
	if (this->file.keys[index] == piece[0].key) {
		// log_info("no need to update piece key %" PRIu64, index);
	} else {
		// log_info("updating piece %" PRIu64 " key from %" PRIu64 " to %" PRIu64,
		// 		index, this->file.keys[index], piece[0].key);
		// char buffer[1024];
		// describe_piece(buffer, this, index);
		// log_info("piece: %s", buffer);
		this->file.keys[index] = piece[0].key;
		cobt_tree_refresh(&this->tree, (cobt_tree_range) {
			.begin = index,
			.end = index + 1
		});
	}
}

static void validate_key(uint64_t key) {
	CHECK(key != COB_INFINITY, "Trying to operate on key=COB_INFINITY");
}

static bool delete_from_piece(cob* this, piece_item* piece, uint64_t key) {
	for (uint8_t i = 0; i < this->piece; i++) {
		if (piece[i].key == key) {
			for (uint8_t j = i; j < this->piece - 1; j++) {
				piece[j].key = piece[j + 1].key;
				piece[j].value = piece[j + 1].value;
			}
			piece[this->piece - 1].key = EMPTY;
			piece[this->piece - 1].value = EMPTY;
			return true;
		}
	}
	return false;
}

// TODO: I could merge the key and the first piece item to save an uint64_t
// for every block.
static void insert_into_piece(cob* this, uint64_t index,
		uint64_t key, uint64_t value) {
	piece_item* piece = get_piece_start(this, index);
	CHECK(piece[this->piece - 1].key == EMPTY, "inserting in full piece");
	for (uint8_t i = 0; i < this->piece - 1; i++) {
		uint8_t idx = (this->piece - 1) - i;
		if (piece[idx - 1].key < key) {
			piece[idx].key = key;
			piece[idx].value = value;
			return;
		} else {
			piece[idx].key = piece[idx - 1].key;
			piece[idx].value = piece[idx - 1].value;
		}
	}
	piece[0].key = key;
	piece[0].value = value;
}

static void clear_piece(cob* this, piece_item* piece) {
	for (uint8_t i = 0; i < this->piece; i++) {
		piece[i].key = EMPTY;
		piece[i].value = EMPTY;
	}
}

static piece_item* new_piece2(uint64_t size) {
	piece_item* piece = malloc(size * sizeof(piece_item));
	assert(piece != NULL);
	for (uint8_t i = 0; i < size; i++) {
		piece[i].key = EMPTY;
		piece[i].value = EMPTY;
	}
	return piece;
}

static piece_item* new_piece(cob* this) {
	return new_piece2(this->piece);
}

static void split_piece(cob* this, uint64_t index) {
	piece_item* old_piece = get_piece_start(this, index);

	if (piece_size(this, old_piece) < this->piece - 1) {
		// The piece still has some free slots, it needs no splitting.
		return;
	}

	// log_info("splitting piece %" PRIu64, index);

	piece_item* piece = new_piece(this);
	const uint8_t start = this->piece / 2;
	for (uint8_t i = start; i < this->piece; i++) {
		piece[i - start].key = old_piece[i].key;
		piece[i - start].value = old_piece[i].value;

		old_piece[i].key = EMPTY;
		old_piece[i].value = EMPTY;
	}

	uint64_t after;
	for (after = index + 1; after < this->file.capacity; after++) {
		if (this->file.occupied[after]) {
			break;
		}
	}
	insert_piece_before(this, piece, after);

	// log_info("After split:");
	// dump_all(this);
}

int8_t cob_insert(cob* this, uint64_t key, uint64_t value) {
	validate_key(key);
	enforce_piece_policy(this, this->size + 1);

	const uint64_t index = cobt_tree_find_le(&this->tree, key);

	piece_item* piece = get_piece_start(this, index);
	IF_LOG_VERBOSE(2) {
		char buffer[1024] = {0};
		describe_piece(buffer, this, index);
		log_info("inserting key %" PRIu64 " to piece %s", key, buffer);
	}
	if (this->file.occupied[index]) {
		for (uint8_t i = 0; i < this->piece; i++) {
			if (piece[i].key == key) {
				// Duplicate key.
				goto duplicate_key;
			}
		}
		insert_into_piece(this, index, key, value);
		refresh_piece_key(this, index);

		// We may need a split, which can invalidate our
		// old pointer.
		split_piece(this, index);
	} else {
		assert(this->size == 0);
		// Special case: create new piece.
		piece_item* piece = new_piece(this);
		piece[0].key = key;
		piece[0].value = value;
		insert_piece_before(this, piece, this->file.capacity);
	}
	++this->size;

	log_verbose(1, "cob_insert(%" PRIu64 "=%" PRIu64 "): done", key, value);
	// internal_check(this);
	return 0;

duplicate_key:
	log_verbose(1, "cob_insert(%" PRIu64 "=%" PRIu64 "): duplicate",
			key, value);
	// internal_check(this);
	return 1;
}

static void merge_pieces(cob* this, uint64_t left, uint64_t right) {
	piece_item* l = get_piece_start(this, left),
			*r = get_piece_start(this, right);
	const uint8_t left_size = piece_size(this, l),
		      right_size = piece_size(this, r);

	if (left_size + right_size < (this->piece * 3) / 4) {
		// Merge right into left, drop right.
		for (uint8_t i = 0; i < right_size; i++) {
			l[i + left_size].key = r[i].key;
			l[i + left_size].value = r[i].value;

			r[i].key = EMPTY;
			r[i].value = EMPTY;
		}

		refresh_piece_key(this, left);
		refresh_piece_key(this, right);

		delete_piece(this, right);
	} else {
		// Redistribute keys between left and right.
		// TODO: fuj
		piece_item* items = alloca(
				(left_size + right_size) * sizeof(piece_item));
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
		for (uint8_t i = 0; i < new_l; i++) {
			l[i].key = items[i].key;
			l[i].value = items[i].value;
		}
		for (uint8_t i = 0; i < new_r; i++) {
			r[i].key = items[new_l + i].key;
			r[i].value = items[new_l + i].value;
		}

		refresh_piece_key(this, left);
		refresh_piece_key(this, right);
	}
}

static void merge_piece(cob* this, uint64_t piece_index) {
	piece_item* piece = get_piece_start(this, piece_index);
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
	if (piece_size(this, piece) == 0) {
		delete_piece(this, piece_index);
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
		goto no_such_key;
	}

	piece_item* piece = get_piece_start(this, index);
	if (!delete_from_piece(this, piece, key)) {
		goto no_such_key;
	}

	// log_info("updating piece key to reflect deletion of %" PRIu64, key);
	refresh_piece_key(this, index);

	// log_info("merging piece with neighbours");
	merge_piece(this, index);
	--this->size;
	log_verbose(1, "cob_delete(%" PRIu64 "): done", key);
	// dump_all(this);
	// internal_check(this);
	return 0;

no_such_key:
	log_verbose(1, "cob_delete(%" PRIu64 "): no such key", key);
	// internal_check(this);
	return 1;
}

void cob_find(cob* this, uint64_t key, bool *found, uint64_t *value) {
	validate_key(key);
	const uint64_t index = cobt_tree_find_le(&this->tree, key);
	if (this->file.occupied[index]) {
		// Look up the key in this piece.
		piece_item* piece = get_piece_start(this, index);
		for (uint8_t i = 0; i < this->piece; i++) {
			if (piece[i].key == key) {
				if (value != NULL) {
					*value = piece[i].value;
				}
				log_verbose(1, "cob_find(%" PRIu64 "): found %" PRIu64,
						key, piece[i].value);
				*found = true;
				return;
			}
		}
	}
	log_verbose(1, "cob_find(%" PRIu64 "): no such key", key);
	*found = false;
}

void cob_next_key(cob* this, uint64_t key,
		bool *next_key_exists, uint64_t* next_key) {
	validate_key(key);
	const uint64_t index = cobt_tree_find_le(&this->tree, key);

	if (this->file.occupied[index]) {
		// Look for next key within the piece.
		piece_item* piece = get_piece_start(this, index);
		for (uint8_t i = 0; i < this->piece; i++) {
			if (piece[i].key > key && piece[i].key != EMPTY) {
				if (next_key) {
					*next_key = piece[i].key;
				}
				*next_key_exists = true;
				return;
			}
		}
	}
	// Look in pieces to the right.
	for (uint64_t i = index + 1; i < this->file.capacity; i++) {
		if (!this->file.occupied[i]) {
			continue;
		}
		piece_item* piece = get_piece_start(this, i);
		if (piece[0].key != EMPTY) {
			if (next_key) {
				*next_key = piece[0].key;
			}
			*next_key_exists = true;
			return;
		}
	}
	*next_key_exists = false;
}

void cob_previous_key(cob* this, uint64_t key,
		bool *previous_key_exists, uint64_t* previous_key) {
	validate_key(key);
	const uint64_t index = cobt_tree_find_le(&this->tree, key);

	if (this->file.occupied[index]) {
		// Look for next key within the piece.
		piece_item* piece = get_piece_start(this, index);
		for (uint8_t i = 0; i < this->piece; i++) {
			uint8_t idx = this->piece - i - 1;
			if (piece[idx].key < key && piece[idx].key != EMPTY) {
				if (previous_key) {
					*previous_key = piece[idx].key;
				}
				*previous_key_exists = true;
				return;
			}
		}
	}
	// Look in pieces to the right.
	for (uint64_t i = 0; i < index; i++) {
		uint64_t idx = index - 1 - i;
		if (!this->file.occupied[idx]) {
			continue;
		}
		piece_item* piece = get_piece_start(this, idx);
		for (uint8_t i = 0; i < this->piece; i++) {
			uint8_t piece_idx = this->piece - i - 1;
			if (piece[piece_idx].key != EMPTY) {
				if (previous_key) {
					*previous_key = piece[piece_idx].key;
				}
				*previous_key_exists = true;
				return;
			}
		}
	}
	*previous_key_exists = false;
}

static pma rebuild_file(cob* this, uint64_t new_size, uint8_t new_piece) {
	pma new_file = { .occupied = NULL, .keys = NULL, .values = NULL };
	const uint8_t preferred_piece = new_piece / 2;
	const uint64_t piece_count = (new_size / preferred_piece) + 1;
	log_info("preparing to store %" PRIu64 " pieces of size %" PRIu8,
			piece_count, new_piece);
	pma_stream stream;
	pma_stream_start(&new_file, piece_count, &stream);

	uint8_t buffer_size = 0;
	piece_item* buffer = new_piece2(new_piece);

	for (uint64_t i = 0; i < this->file.capacity; i++) {
		if (!this->file.occupied[i]) {
			continue;
		}
		piece_item* this_piece = get_piece_start(this, i);
		for (uint8_t j = 0; j < this->piece; j++) {
			if (this_piece[j].key == EMPTY) {
				continue;
			}
			assert(buffer_size < preferred_piece);
			buffer[buffer_size].key = this_piece[j].key;
			buffer[buffer_size].value = this_piece[j].value;
			++buffer_size;

			log_verbose(2, "push %" PRIu64 "=%" PRIu64,
					this_piece[j].key, this_piece[j].value);

			if (buffer_size == preferred_piece) {
				log_verbose(2, "flush");
				pma_stream_push(&stream, buffer[0].key, buffer);
				buffer_size = 0;
				buffer = new_piece2(new_piece);
			}
		}
		free(this_piece);
	}

	if (buffer_size > 0) {
		log_verbose(2, "final flush");
		pma_stream_push(&stream, buffer[0].key, buffer);
	} else {
		free(buffer);
	}
	log_verbose(1, "rebuilt file to piece %" PRIu8, new_piece);
	return new_file;
}

void cob_init(cob* this) {
	log_info("cob_init(%p)", this);
	this->size = 0;
	this->piece = 4;  // Initial piece size: 4
	pma_init(&this->file);
	cobt_tree_init(&this->tree, this->file.keys, this->file.occupied,
			this->file.capacity);
}

void cob_destroy(cob* this) {
	for (uint64_t i = 0; i < this->file.capacity; i++) {
		if (this->file.occupied[i]) {
			free(get_piece_start(this, i));
		}
	}
	pma_destroy(&this->file);
	cobt_tree_destroy(&this->tree);
}
