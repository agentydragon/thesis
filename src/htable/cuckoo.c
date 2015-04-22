#include "htable/cuckoo.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "htable/hash.h"
//#define NO_LOG_INFO
#include "log/log.h"
#include "util/unused.h"

// TODO: MIN_SIZE, too_sparse, too_dense, pick_capacity partially
// duplicated with htable.c
static const uint64_t MIN_SIZE = 2;

static bool too_sparse(uint64_t pairs, uint64_t capacity) {
	// Keep at least MIN_SIZE slots.
	if (capacity <= MIN_SIZE) {
		return false;
	}
	// At least one quarter full.
	return pairs * 4 < capacity;
}

static bool too_dense(uint64_t pairs, uint64_t capacity) {
	// Keep at least MIN_SIZE slots.
	if (capacity < MIN_SIZE) {
		return true;
	}
	// At most three quarters full.
	return pairs * 4 > capacity * 3;
}

static uint64_t pick_capacity(uint64_t current_size, uint64_t pairs) {
	uint64_t new_size = current_size;

	while (too_sparse(pairs, new_size)) {
		new_size /= 2;
	}

	while (too_dense(pairs, new_size)) {
		new_size *= 2;
		if (new_size < MIN_SIZE) {
			new_size = MIN_SIZE;
		}
	}

	assert(!too_sparse(pairs, new_size) && !too_dense(pairs, new_size));
	return new_size;
}

static void pick_new_hash_fn(htcuckoo* this, cuckoo_half* half,
		rand_generator* rand) {
	sth_init(&half->hash, this->half_capacity, rand);
}

static uint64_t half_hash(cuckoo_half* half, uint64_t key) {
	return sth_hash(&half->hash, key);
}

const uint64_t UNVISITED = UINT64_MAX, SENTINEL = (UINT64_MAX - 1);

static void clear_half(cuckoo_half* half, uint64_t capacity) {
	for (uint64_t i = 0; i < capacity; ++i) {
		half->keys[i] = CUCKOO_EMPTY;
		half->backptr[i] = UNVISITED;
	}
}

static void allocate_half(cuckoo_half* half, uint64_t capacity) {
	half->keys = malloc(sizeof(uint64_t) * capacity);
	half->values = malloc(sizeof(uint64_t) * capacity);
	half->backptr = malloc(sizeof(uint64_t) * capacity);
	ASSERT(half->keys && half->values && half->backptr);
	clear_half(half, capacity);
}

static void destroy_half(cuckoo_half* half) {
	free(half->keys);
	free(half->values);
}

void htcuckoo_init(htcuckoo* this, rand_generator rand) {
	this->rand = rand;
	this->pair_count = 0;
	this->half_capacity = 2;
	allocate_half(&this->left, 2);
	allocate_half(&this->right, 2);
	pick_new_hash_fn(this, &this->left, &this->rand);
	pick_new_hash_fn(this, &this->right, &this->rand);
}

void htcuckoo_destroy(htcuckoo* this) {
	destroy_half(&this->left);
	destroy_half(&this->right);
}

typedef enum { LEFT, RIGHT } half_t;

static void swap_halves(cuckoo_half** x, cuckoo_half** y) {
	cuckoo_half* tmp = *x;
	*x = *y;
	*y = tmp;
}

static void select_half(htcuckoo* this,
		cuckoo_half** this_half, cuckoo_half** other_half,
		half_t which) {
	switch (which) {
	case LEFT:
		*this_half = &this->left; *other_half = &this->right; break;
	case RIGHT:
		*this_half = &this->right; *other_half = &this->right; break;
	default: ASSERT(false);
	}
}

static UNUSED void dump_half(cuckoo_half* half, uint64_t capacity) {
	for (uint64_t i = 0; i < capacity; ++i) {
		char content[128], back[128];
		if (half->keys[i] == CUCKOO_EMPTY) {
			sprintf(content, "nil(=%" PRIu64 ")", half->values[i]);
		} else {
			sprintf(content, "%" PRIu64 "=%" PRIu64,
					half->keys[i], half->values[i]);
		}
		if (half->backptr[i] == UNVISITED) {
			strcpy(back, "unvisited");
		} else if (half->backptr[i] == SENTINEL) {
			strcpy(back, "sentinel");
		} else {
			sprintf(back, "back=%" PRIu64, half->backptr[i]);
		}
		log_info("%04" PRIu64 " %10s %10s", i, content, back);
	}
}

static UNUSED void dump(htcuckoo* this) {
	(void) this;
	//log_info("LEFT:");
	//dump_half(&this->left, this->half_capacity);
	//log_info("RIGHT:");
	//dump_half(&this->right, this->half_capacity);
}

static void evict(htcuckoo* this, const half_t which_half,
		const uint64_t vh_slot_index) {
	cuckoo_half *this_half, *other_half;
	select_half(this, &this_half, &other_half, which_half);
	uint64_t slot_index = vh_slot_index;
	uint64_t key = this_half->keys[slot_index],
			 value = this_half->values[slot_index];
	this_half->keys[slot_index] = CUCKOO_EMPTY;
	while (true) {
		const uint64_t slot_index2 = half_hash(other_half, key);
		const uint64_t key2 = other_half->keys[slot_index2],
				 value2 = other_half->values[slot_index2];
		const bool finished =
				(other_half->keys[slot_index2] == CUCKOO_EMPTY);
		other_half->keys[slot_index2] = key;
		other_half->values[slot_index2] = value;
		key = key2; value = value2; slot_index = slot_index2;

		if (finished) {
			break;
		}
		swap_halves(&this_half, &other_half);
	}

	dump(this);
}

static void clear_backptr(cuckoo_half* this_half, cuckoo_half* other_half,
		uint64_t slot_index) {
	while (this_half->backptr[slot_index] != SENTINEL) {
		const uint64_t slot_index2 = this_half->backptr[slot_index];
		this_half->backptr[slot_index] = UNVISITED;
		swap_halves(&this_half, &other_half);
		slot_index = slot_index2;
	}
	this_half->backptr[slot_index] = UNVISITED;
}

static bool try_vacate(htcuckoo* this, const half_t which_half,
		const uint64_t vh_slot_index) {
	log_info("vacating %d slot %" PRIu64, which_half, vh_slot_index);
	dump(this);

	cuckoo_half *this_half, *other_half;
	select_half(this, &this_half, &other_half, which_half);
	uint64_t slot_index = vh_slot_index;
	if (this_half->keys[slot_index] == CUCKOO_EMPTY) {
		return true;
	}
	this_half->backptr[slot_index] = SENTINEL;

	while (true) {
		const uint64_t key = this_half->keys[slot_index];
		const uint64_t slot_index2 = half_hash(other_half, key);
		const uint64_t hash_l = half_hash(&this->left, key),
			      hash_r = half_hash(&this->right, key);
		(void) hash_l; (void) hash_r;
		log_info("=> %" PRIu64 "[L:%" PRIx64 " R:%" PRIx64 "] @ %" PRIu64,
				key, hash_l, hash_r, slot_index2);
		if (other_half->keys[slot_index2] == CUCKOO_EMPTY) {
			// Great! Let's execute this!
			log_info("clear, evicting");
			evict(this, which_half, vh_slot_index);
			clear_backptr(this_half, other_half, slot_index);
			return true;
		}

		if (other_half->backptr[slot_index2] != UNVISITED) {
			// Found cycle :(
			clear_backptr(this_half, other_half, slot_index);
			log_info("found cycle");
			return false;
		}
		other_half->backptr[slot_index2] = slot_index;
		swap_halves(&this_half, &other_half);
		slot_index = slot_index2;
	}
}

static bool insert_norebuild(htcuckoo* this, uint64_t key, uint64_t value) {
	const uint64_t hash_l = half_hash(&this->left, key),
		      hash_r = half_hash(&this->right, key);
	log_info("insert_norebuild(%" PRIu64 "[L:%" PRIx64 " R:%" PRIx64 "]="
			"%" PRIu64 ")", key, hash_l, hash_r, value);
	// TODO: The right thing to do would probably be progressing in parallel
	// steps to ensure we don't do too much work.
	// (Or maybe just randomizing L/R order.)
	if (try_vacate(this, LEFT, hash_l)) {
		ASSERT(this->left.keys[hash_l] == CUCKOO_EMPTY);
		this->left.keys[hash_l] = key;
		this->left.values[hash_l] = value;
		return true;
	}
	if (try_vacate(this, RIGHT, hash_r)) {
		ASSERT(this->right.keys[hash_r] == CUCKOO_EMPTY);
		this->right.keys[hash_r] = key;
		this->right.values[hash_r] = value;
		return true;
	}
	return false;
}

static bool copy_half(cuckoo_half* half, uint64_t half_capacity,
		htcuckoo* target) {
	for (uint64_t i = 0; i < half_capacity; ++i) {
		if (half->keys[i] != CUCKOO_EMPTY) {
			const uint64_t key = half->keys[i],
					value = half->values[i];
			if (!insert_norebuild(target, key, value)) {
				// Cycle :(
				log_info("%" PRIu64 "=%" PRIu64 " is cycle",
						key, value);
				return false;
			}
		}
	}
	return true;
}

static void refit(htcuckoo* this, uint64_t half_capacity) {
	ASSERT(half_capacity * 2 > this->pair_count);

	// Cannot use realloc, because this can both upscale and downscale.
	htcuckoo new_this = {
		.pair_count = this->pair_count,
		.half_capacity = half_capacity,
		.rand = this->rand
	};
	allocate_half(&new_this.left, half_capacity);
	allocate_half(&new_this.right, half_capacity);

	for (uint64_t rebuilds = 0; ; ++rebuilds) {
		clear_half(&new_this.left, half_capacity);
		clear_half(&new_this.right, half_capacity);
		pick_new_hash_fn(&new_this, &new_this.left, &this->rand);
		pick_new_hash_fn(&new_this, &new_this.right, &this->rand);

		log_info("picked new hash fns, half_capacity=%" PRIu64 " pc=%" PRIu64,
				half_capacity, this->pair_count);

		if (!copy_half(&this->left, this->half_capacity, &new_this)) {
			// Cycle :(
			log_info("cycle copying left half");
			continue;
		}
		if (!copy_half(&this->right, this->half_capacity, &new_this)) {
			// Cycle :(
			log_info("cycle copying right half");
			continue;
		}
		CHECK(rebuilds < 10, "suspiciously many rebuilds");
		break;
	}

	htcuckoo_destroy(this);
	*this = new_this;
}

static void resize_to_fit(htcuckoo* this, uint64_t to_fit) {
	const uint64_t new_capacity = pick_capacity(
			this->half_capacity * 2, to_fit);

	// TODO: make this operation a takeback instead?
	if (new_capacity == this->half_capacity * 2) {
		return;  // No resizing needed.
	}
	log_info("will resize to %" PRIu64 " to fit %" PRIu64,
			new_capacity, to_fit);
	refit(this, new_capacity / 2);
}

int8_t htcuckoo_delete(htcuckoo* this, uint64_t key) {
	const uint64_t hash_l = half_hash(&this->left, key),
			hash_r = half_hash(&this->right, key);
	// TODO: Ideally we'd like this to be done as 2 parallel lookups.
	// Can we force that to happen?
	if (this->left.keys[hash_l] == key) {
		this->left.keys[hash_l] = CUCKOO_EMPTY;
	} else if (this->right.keys[hash_r] == key) {
		this->right.keys[hash_r] = CUCKOO_EMPTY;
	} else {
		return 1;  // No such key.
	}
	resize_to_fit(this, --this->pair_count);
	return 0;
}

bool htcuckoo_find(htcuckoo* this, uint64_t key, uint64_t *value) {
	log_info("find(%" PRIu64 ")", key);
	const uint64_t hash_l = half_hash(&this->left, key),
			hash_r = half_hash(&this->right, key);
	// TODO: Ideally we'd like this to be done as 2 parallel lookups.
	// Can we force that to happen?
	if (this->left.keys[hash_l] == key) {
		if (value) {
			*value = this->left.values[hash_l];
		}
		return true;
	}
	if (this->right.keys[hash_r] == key) {
		if (value) {
			*value = this->right.values[hash_r];
		}
		return true;
	}
	return false;
}

int8_t htcuckoo_insert(htcuckoo* this, uint64_t key, uint64_t value) {
	log_info("insert(%" PRIu64 "=%" PRIu64 ")", key, value);
	if (htcuckoo_find(this, key, NULL)) {
		return 1;
	}
	resize_to_fit(this, ++this->pair_count);
	uint64_t rebuilds = 0;
	while (true) {
		if (insert_norebuild(this, key, value)) {
			// We are happy.
			return 0;
		}
		const uint64_t hash_l = half_hash(&this->left, key),
			      hash_r = half_hash(&this->right, key);
		(void) hash_l; (void) hash_r;
		log_info("unhappy %" PRIu64 "[L:%" PRIx64 " R:%" PRIx64 "]="
				"%" PRIu64, key, hash_l, hash_r, value);
		// Need to rehash.
		// TODO: Rehash in-place.
		refit(this, this->half_capacity);
		++rebuilds;

		CHECK(rebuilds < 10, "suspiciously many rebuilds");
	}
}
