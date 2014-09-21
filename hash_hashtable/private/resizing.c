#include "resizing.h"
#include "insertion.h"

#include <inttypes.h>
#include <string.h>
#include <assert.h>

#define NO_LOG_INFO

#include "../../log/log.h"

static const uint64_t MIN_SIZE = 4; // 8;

static bool too_sparse(uint64_t pairs, uint64_t buckets) {
	// Keep at least MIN_SIZE buckets.
	if (buckets <= MIN_SIZE) return false;

	// At least one quarter full.
	return pairs * 4 < buckets;
}

static bool too_dense(uint64_t pairs, uint64_t buckets) {
	// Keep at least MIN_SIZE buckets.
	if (buckets < MIN_SIZE) return true;

	// At most three quarters full.
	return pairs * 4 > buckets * 3;
}

static uint64_t next_bigger_size(uint64_t x) {
	if (x < MIN_SIZE) return MIN_SIZE;

	// Next power of 2
	uint64_t i = 1;
	while (i <= x) i *= 2;
	return i;
}

static uint64_t next_smaller_size(uint64_t x) {
	// Previous power of 2
	uint64_t i = 1;
	while (i < x) i *= 2;
	return i / 2;
}

static int8_t resize(struct hashtable_data* this, uint64_t new_size);

int8_t hashtable_resize_to_fit(struct hashtable_data* this, uint64_t to_fit) {
	uint64_t new_size = this->table_size;

	while (too_sparse(to_fit, new_size)) {
		new_size = next_smaller_size(new_size);
	}

	while (too_dense(to_fit, new_size)) {
		new_size = next_bigger_size(new_size);
	}

	assert(!too_sparse(to_fit, new_size) && !too_dense(to_fit, new_size));

	// TODO: make this operation a takeback instead?
	if (new_size != this->table_size) {
		if (resize(this, new_size)) {
			log_error("failed to resize");
			return 1;
		}
	}

	return 0;
}

// TODO: make public?
// TODO: allow break?
static int8_t foreach(struct hashtable_data* this, int8_t (*iterate)(void*, uint64_t key, uint64_t value), void* opaque) {
	for (uint64_t i = 0; i < this->table_size; i++) {
		const struct hashtable_bucket *bucket = &this->table[i];
		if (bucket->occupied) {
			if (iterate(opaque, bucket->key, bucket->value)) {
				log_error("foreach iteration failed on %ld=%ld", bucket->key, bucket->value);
				return 1;
			}
		}
	}
	return 0;
}

static int8_t insert_internal_wrap(void* this, uint64_t key, uint64_t value) {
	return hashtable_insert_internal(this, key, value);
}

static int8_t resize(struct hashtable_data* this, uint64_t new_size) {
	if (new_size < this->pair_count) {
		log_error("cannot resize: target size %" PRIu64 " too small for %" PRIu64 " pairs", new_size, this->pair_count);
		goto err_1;
	}
	// TODO: try new hash function in this case?

	// Cannot use realloc, because this can both upscale and downscale.
	struct hashtable_data new_this = {
		.table = malloc(sizeof(struct hashtable_bucket) * new_size),
		.table_size = new_size,
		.pair_count = 0
	};

	log_info("resizing to %" PRIu64, new_this.table_size);

	if (!new_this.table) {
		log_error("cannot allocate memory for %ld buckets", new_size);
		goto err_1;
	}

	memset(new_this.table, 0, sizeof(struct hashtable_bucket) * new_size);

	if (foreach(this, insert_internal_wrap, &new_this)) {
		log_error("iteration failed");
		goto err_2;
	}

	free(this->table);
	this->table = new_this.table;
	this->table_size = new_this.table_size;

	return 0;

err_2:
	free(new_this.table);
err_1:
	return 1;
}
