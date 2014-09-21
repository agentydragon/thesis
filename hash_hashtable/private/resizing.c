#include "resizing.h"
#include "insertion.h"

#include <inttypes.h>
#include <string.h>

#define NO_LOG_INFO

#include "../../log/log.h"

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

int8_t hashtable_resize(struct hashtable_data* this, uint64_t new_size) {
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
