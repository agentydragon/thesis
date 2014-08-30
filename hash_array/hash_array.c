#include "hash_array.h"

#include "../log/log.h"

#include <stdlib.h>
#include <string.h>

// TODO: do not malloc

struct pair {
	uint64_t key;
	uint64_t value;
};

struct data {
	struct pair* pairs;
	uint64_t pair_count;
	uint64_t pair_capacity;
};

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	struct data* this = malloc(sizeof(struct data));
	if (!this) return 1;
	memcpy(this, & (struct data) {
		.pairs = NULL,
		.pair_count = 0,
		.pair_capacity = 0
	}, sizeof(struct data));
	*_this = this;
	return 0;
}

static void destroy(void** _this) {
	if (_this) {
		struct data* this = *_this;
		free(this->pairs);
		free(this);
		*_this = NULL;
	}
}

static void _lookup_index(struct data* this, uint64_t key, bool *found, uint64_t *index) {
	for (uint64_t i = 0; i < this->pair_count; i++) {
		if (this->pairs[i].key == key) {
			*found = true;
			*index = i;
			return;
		}
	}
	*found = false;
}

static int8_t find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	struct data* this = _this;
	uint64_t index;

	bool pair_found;
	_lookup_index(this, key, &pair_found, &index);
	if (found) *found = pair_found;
	if (pair_found && value) *value = this->pairs[index].value;

	return 0;
}

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	struct data* this = _this;

	bool found;
	if (find(this, key, NULL, &found)) {
		log_error("cannot check for duplicity when inserting %ld=%ld", key, value);
		goto err_1;
	}

	if (found) {
		log_error("value for key %ld already present when inserting %ld=%ld", key, key, value);
		goto err_1;
	}

	if (this->pair_count == this->pair_capacity) {
		uint64_t new_capacity;
		if (this->pair_capacity == 0) {
			new_capacity = 1;
		} else {
			new_capacity = this->pair_capacity * 2;
		}

		struct pair* new_pairs = realloc(this->pairs, sizeof(struct pair) * new_capacity);
		if (!new_pairs) {
			goto err_1;
		}

		this->pairs = new_pairs;
		this->pair_capacity = new_capacity;
	}

	memcpy(&this->pairs[this->pair_count++], & (struct pair) {
		.key = key,
		.value = value
	}, sizeof(struct pair));
	return 0;

err_1:
	return 1;
}

static int8_t delete(void* _this, uint64_t key) {
	struct data* this = _this;

	bool found;
	uint64_t index;

	_lookup_index(this, key, &found, &index);
	if (!found) {
		// ERROR: no such element
		return 1;
	}
	memmove(&this->pairs[index], &this->pairs[index + 1], sizeof(struct pair) * (this->pair_count - index - 1));
	this->pair_count--;

	return 0;
}

const hash_api hash_array = {
	.init = init,
	.destroy = destroy,

	.insert = insert,
	.find = find,
	.delete = delete,

	.name = "hash_array"
};
