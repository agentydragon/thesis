#include "dump.h"
#include "data.h"
#include "hash.h"
#include "traversal.h"

#include "../../util/average.h"
#include "../../log/log.h"

#include <inttypes.h>
#include <string.h>
#include <assert.h>

/*
static void calculate_bucket_sizes(struct hashtable_data* this, int bucket_sizes[100]) {
	memset(bucket_sizes, 0, sizeof(int) * 100);
	for (uint64_t i = 0; i < this->table_size; i++) {
		uint64_t bucket_size = this->table[i].keys_with_hash;
		assert(bucket_size < 100);
		bucket_sizes[bucket_size]++;
	}
}

static void dump_bucket(struct hashtable_data* this, uint64_t index, struct hashtable_bucket* bucket) {
	if (bucket->occupied) {
		log_plain("[%04" PRIu64 "] keys_with_hash=%" PRIu32 " occupied, %" PRIu64 "(h=%" PRIu64 ")=%" PRIu64,
			index,
			bucket->keys_with_hash,
			bucket->key,
			hash_of(this, bucket->key),
			bucket->value
		);
	} else {
		log_plain("[%04" PRIu64 "] keys_with_hash=%" PRIu32 " not occupied",
			index,
			bucket->keys_with_hash
		);
	}
}

void hashtable_dump(void* _this) {
	struct hashtable_data* this = _this;

	log_plain("hash_hashtable table_size=%ld pair_count=%ld",
		this->table_size,
		this->pair_count);

	int bucket_sizes[100] = { 0 };
	int distances[100] = { 0 };

	calculate_bucket_sizes(this, bucket_sizes);

	for (int i = 0; i < 100; i++) {
		if (bucket_sizes[i] > 0) {
			log_plain("%d same-hash groups of size %d", bucket_sizes[i], i);
		}
	}
	log_plain("average bucket size: %.2lf", histogram_average_i(bucket_sizes, 100));
	for (uint64_t i = 0; i < this->table_size; i++) {
		dump_bucket(this, i, &this->table[i]);
	}
}
*/

static void calculate_distances(struct hashtable_data* this, int distances[100]) {
	memset(distances, 0, sizeof(int) * 100);
	// TODO: optimize?
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		for (int subindex = 0; subindex < 3; subindex++) {
			if (!this->blocks[i].occupied[subindex]) continue;

			uint64_t should_be_at = hashtable_hash_of(this, this->blocks[i].keys[subindex]);

			uint64_t distance;
			if (should_be_at <= i) distance = i - should_be_at;
			else distance = (this->blocks_size - should_be_at) + i;
			distances[distance]++;
		}
	}
}

void hashtable_dump(void* _this) {
	struct hashtable_data* this = _this;

	log_plain("hash_hashtable blocks:%ld pair_count:%ld",
			this->blocks_size, this->pair_count);

	{
		int distances[100] = { 0 };
		calculate_distances(this, distances);
		log_plain("average distance: %.2lf", histogram_average_i(distances, 100));
		for (int i = 0; i < 100; i++) {
			if (distances[i] > 0) {
				log_plain("distance %3d: %8d groups", i, distances[i]);
			}
		}
	}
}
