#include "htable/htable.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "htable/private/hash.h"
#include "htable/private/traversal.h"
#include "log/log.h"
#include "util/average.h"
#include "util/unused.h"

/*
static void calculate_bucket_sizes(htable* this, int bucket_sizes[100]) {
	memset(bucket_sizes, 0, sizeof(int) * 100);
	for (uint64_t i = 0; i < this->table_size; i++) {
		uint64_t bucket_size = this->table[i].keys_with_hash;
		assert(bucket_size < 100);
		bucket_sizes[bucket_size]++;
	}
}

void htable_dump(void* _this) {
	htable* this = _this;

	log_plain("htable table_size=%ld pair_count=%ld",
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
}
*/

// Used for debugging. TODO: remove?
static void UNUSED dump_block(htable* this, uint64_t index,
		htable_block* block) {
	char buffer[256], buffer2[256];
	snprintf(buffer, sizeof(buffer),
			"[%04" PRIx64 "] keys_with_hash=%" PRIu32,
			index, block->keys_with_hash);

	for (int i = 0; i < 3; i++) {
		if (block->occupied[i]) {
			strncpy(buffer2, buffer, sizeof(buffer2) - 1);
			snprintf(buffer, sizeof(buffer),
					"%s [%016" PRIx64 "(%04" PRIx64 ")=%016" PRIx64 "]",
					buffer2,
					block->keys[i],
					hash_of(this, block->keys[i]),
					block->values[i]);
		} else {
			strncpy(buffer2, buffer, sizeof(buffer2) - 1);
			snprintf(buffer, sizeof(buffer),
					"%s [                (    )=                ]",
					buffer2);
		}
	}

	log_plain("%s", buffer);
}

static void UNUSED dump_blocks(htable* this) {
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		dump_block(this, i, &this->blocks[i]);
	}
}

static void calculate_distances(htable* this, int distances[100]) {
	memset(distances, 0, sizeof(int) * 100);
	// TODO: optimize?
	for (uint64_t i = 0; i < this->blocks_size; i++) {
		for (int8_t slot = 0; slot < 3; slot++) {
			if (!this->blocks[i].occupied[slot]) continue;

			const uint64_t should_be_at =
				hash_of(this, this->blocks[i].keys[slot]);

			uint64_t distance;
			if (should_be_at <= i) {
				distance = i - should_be_at;
			} else {
				distance = (this->blocks_size - should_be_at) + i;
			}
			distances[distance]++;
		}
	}
}

static void dump_distances(htable* this) {
	if (this->blocks_size > 0) {
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

void htable_dump(void* _this) {
	htable* this = _this;

	log_plain("htable blocks:%ld pair_count:%ld",
			this->blocks_size, this->pair_count);
	// dump_blocks(this);
	dump_distances(this);
}
