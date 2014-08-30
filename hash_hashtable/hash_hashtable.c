#include "hash_hashtable.h"

#define NO_LOG_INFO

#include "../log/log.h"

#include <stdlib.h>
#include <string.h>

static const uint64_t MIN_SIZE = 8;

struct bucket {
	// how many keys have this hash?
	uint64_t keys_with_hash;

	// the key stored in this bucket
	bool occupied;
	uint64_t key;
	uint64_t value;
};

struct data {
	struct bucket* table;
	uint64_t table_size;
	uint64_t pair_count;
};

uint64_t PRIME_X = 433024253L, PRIME_Y = 817504243;

static uint64_t hash_fn(uint64_t x, uint64_t max) {
	return ((x * PRIME_X) ^ (x * PRIME_Y)) % max;
}

static uint64_t hash_of(struct data* this, uint64_t x) {
	uint64_t result = hash_fn(x, this->table_size);
	log_info("hash(%ld) where size=%ld = %ld", x, this->table_size, result);
	return result;
}

static uint64_t next_index(struct data* this, uint64_t i) {
	return (i + 1) % this->table_size;
}

static bool too_sparse(uint64_t pairs, uint64_t buckets) {
	// Keep at least MIN_SIZE buckets.
	if (buckets <= MIN_SIZE) return false;

	// At more than MIN_SIZE buckets, keep at least one third of buckets full.
	return pairs * 3 < buckets;
}

static bool too_dense(uint64_t pairs, uint64_t buckets) {
	// At most two thirds full.
	return pairs * 3 > buckets * 2;
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

static int8_t init(void** _this) {
	struct data* this = malloc(sizeof(struct data));
	if (!this) {
		log_error("cannot allocate new hash_hashtable");
		goto err_1;
	}

	memcpy(this, & (struct data) {
		.table = malloc(sizeof(struct bucket) * MIN_SIZE),
		.table_size = MIN_SIZE,
		.pair_count = 0
	}, sizeof(struct data));

	if (!this->table) {
		log_error("cannot allocate hash table");
		goto err_2;
	}
	memset(this->table, 0, sizeof(struct bucket) * MIN_SIZE);

	*_this = this;

	return 0;

err_2:
	free(this);
err_1:
	return 1;
}

static void destroy(void** _this) {
	if (_this) {
		struct data* this = *_this;
		if (this) {
			free(this->table);
		}
		free(this);
		*_this = NULL;
	}
}

static int8_t find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	struct data* this = _this;

	log_info("find(%ld)", key);

	uint64_t key_hash = hash_of(this, key);

	uint64_t index = key_hash;
	uint64_t keys_with_hash = this->table[index].keys_with_hash;

	for (uint64_t i = 0; i < keys_with_hash; index = next_index(this, index)) {
		if (this->table[index].occupied) {
			if (this->table[index].key == key) {
				*found = true;
				if (value) *value = this->table[index].value;
				log_info("find(%ld): found, value=%ld", key, this->table[index].value);
				return 0;
			}

			if (hash_of(this, this->table[index].key) == key_hash) {
				i++;
			}
		}
		// TODO: guard against infinite loop?
	}

	log_info("find(%ld): not found", key);
	*found = false;
	return 0;
}

// TODO: make public?
// TODO: allow break?
static int8_t foreach(struct data* this, int8_t (*iterate)(void*, uint64_t key, uint64_t value), void* opaque) {
	log_info("entering foreach");
	for (uint64_t i = 0; i < this->table_size; i++) {
		const struct bucket *bucket = &this->table[i];
		if (bucket->occupied) {
			log_info("foreach: %ld=%ld", bucket->key, bucket->value);
			if (iterate(opaque, bucket->key, bucket->value)) {
				log_error("foreach iteration failed on %ld=%ld", bucket->key, bucket->value);
				return 1;
			}
		}
	}
	log_info("foreach complete");
	return 0;
}

static int8_t insert(void* _this, uint64_t key, uint64_t value);

static int8_t resize(struct data* this, uint64_t new_size) {
	log_info("resizing to %ld", new_size);
	// TODO: try new hash function in this case?

	// Cannot use realloc, because this can both upscale and downscale.
	struct data new_this = {
		.table = malloc(sizeof(struct bucket) * new_size),
		.table_size = new_size,
		.pair_count = 0
	};

	if (!new_this.table) {
		log_error("cannot allocate memory for %ld buckets", new_size);
		goto err_1;
	}

	memset(new_this.table, 0, sizeof(struct bucket) * new_size);

	if (foreach(this, insert, &new_this)) {
		goto err_2;
	}

	free(this->table);
	this->table = new_this.table;
	this->table_size = new_size;

	return 0;

err_2:
	free(new_this.table);
err_1:
	return 1;
}

static void dump_bucket(uint64_t index, struct bucket* bucket) {
	if (bucket->occupied) {
		log_plain("[%04ld] keys_with_hash=%ld occupied, %ld=%ld",
			index,
			bucket->keys_with_hash,
			bucket->key,
			bucket->value
		);
	} else {
		log_plain("[%04ld] keys_with_hash=%ld not occupied",
			index,
			bucket->keys_with_hash
		);
	}
}

static void dump(void* _this) {
	struct data* this = _this;

	log_plain("hash_hashtable table_size=%ld pair_count=%ld",
		this->table_size,
		this->pair_count);

	for (uint64_t i = 0; i < this->table_size; i++) {
		dump_bucket(i, &this->table[i]);
	}
}

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	struct data* this = _this;

	log_info("insert(%ld, %ld)", key, value);

	uint64_t new_size = this->table_size;
	while (too_dense(this->pair_count + 1, new_size)) {
		new_size = next_bigger_size(new_size);
	}
	if (new_size != this->table_size) {
		if (resize(this, next_bigger_size(this->table_size))) {
			log_error("failed to resize when inserting %ld=%ld", key, value);
			return 1;
		}
	}

	uint64_t key_hash = hash_of(this, key);

	bool free_index_found = false;
	uint64_t free_index;

	for (uint64_t i = 0, index = key_hash; i < this->table[key_hash].keys_with_hash || !free_index_found; index = next_index(this, index)) {
		if (this->table[index].occupied) {
			if (hash_of(this, this->table[index].key) == key_hash) {
				i++;

				if (this->table[index].key == key) {
					log_error("duplicate when inserting %ld=%ld", key, value);
					return 1;
				}
			}
		} else {
			if (!free_index_found) {
				free_index = index;
				free_index_found = true;
			}
		}
	}

	memcpy(&this->table[free_index], & (struct bucket) {
		.occupied = true,
		.key = key,
		.value = value
	}, sizeof(struct bucket));

	this->table[key_hash].keys_with_hash++;
	this->pair_count++;

	return 0;
}

static int8_t delete(void* _this, uint64_t key) {
	struct data* this = _this;

	log_info("delete(%ld)", key);

	uint64_t new_size = this->table_size;
	while (too_sparse(this->pair_count - 1, new_size)) {
		new_size = next_smaller_size(new_size);
	}
	if (new_size != this->table_size) {
		if (resize(this, next_smaller_size(new_size))) {
			log_error("failed to resize while deleting key %ld", key);
			return 1;
		}
	}

	uint64_t key_hash = hash_of(this, key);

	uint64_t index = key_hash;
	uint64_t keys_with_hash = this->table[key_hash].keys_with_hash;

	for (uint64_t i = 0; i < keys_with_hash; index = next_index(this, index)) {
		if (this->table[index].occupied) {
			if (this->table[index].key == key) {
				this->table[index].occupied = false;
				this->table[key_hash].keys_with_hash--;
				this->pair_count--;
				return 0;
			}

			if (hash_of(this, this->table[index].key) == key_hash) {
				i++;
			}
		}
	}

	log_error("key %ld not present, cannot delete", key);
	return 1;
}

const hash_api hash_hashtable = {
	.init = init,
	.destroy = destroy,

	.find = find,
	.insert = insert,
	.delete = delete,

	.name = "hash_hashtable"
};
