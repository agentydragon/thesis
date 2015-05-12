#include "dict/array.h"

#include "log/log.h"

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

// TODO: do not malloc

typedef struct {
	uint64_t key;
	uint64_t value;
} pair;

typedef struct {
	pair* pairs;
	uint64_t pair_count;
	uint64_t pair_capacity;
} data;

static void init(void** _this) {
	data* this = malloc(sizeof(data));
	CHECK(this, "cannot allocate memory for array dict");
	*this = (data) {
		.pairs = NULL,
		.pair_count = 0,
		.pair_capacity = 0
	};
	*_this = this;
}

static void destroy(void** _this) {
	if (_this) {
		data* this = *_this;
		free(this->pairs);
		free(this);
		*_this = NULL;
	}
}

static bool lookup_index(data* this, uint64_t key, uint64_t *index) {
	uint64_t min_inclusive = 0, max_exclusive = this->pair_count;
	if (this->pair_count == 0) {
		return false;
	}

	while (min_inclusive + 1 < max_exclusive) {
		const uint64_t mid = (min_inclusive + max_exclusive) / 2;
		const uint64_t midpoint = this->pairs[mid].key;
		if (midpoint > key) {
			max_exclusive = mid;
		} else {
			min_inclusive = mid;
		}
	}

	*index = min_inclusive;
	return (this->pairs[min_inclusive].key == key);
}

static bool find(void* _this, uint64_t key, uint64_t *value) {
	data* this = _this;
	uint64_t index;

	const bool found = lookup_index(this, key, &index);
	if (found && value) {
		*value = this->pairs[index].value;
	}
	return found;
}

static bool prev(void* _this, uint64_t key, uint64_t *prev) {
	data* this = _this;
	uint64_t index;

	if (this->pair_count == 0) {
		return false;
	}
	lookup_index(this, key, &index);

	if (this->pairs[index].key < key) {
		*prev = this->pairs[index].key;
		return true;
	} else {
		if (index == 0) {
			return false;
		} else {
			*prev = this->pairs[index - 1].key;
			return true;
		}
	}
}

static bool next(void* _this, uint64_t key, uint64_t *next) {
	data* this = _this;
	uint64_t index;

	if (this->pair_count == 0) {
		return false;
	}
	lookup_index(this, key, &index);

	if (this->pairs[index].key > key) {
		*next = this->pairs[index].key;
		return true;
	}
	if (index == this->pair_count - 1) {
		return false;
	} else {
		*next = this->pairs[index + 1].key;
		return true;
	}
}

static bool insert(void* _this, uint64_t key, uint64_t value) {
	data* this = _this;

	if (this->pair_count == this->pair_capacity) {
		uint64_t new_capacity;
		if (this->pair_capacity == 0) {
			new_capacity = 1;
		} else {
			new_capacity = this->pair_capacity * 2;
		}

		pair* new_pairs = realloc(this->pairs,
				sizeof(pair) * new_capacity);
		if (!new_pairs) {
			return false;
		}

		this->pairs = new_pairs;
		this->pair_capacity = new_capacity;
	}

	const pair new_pair = { .key = key, .value = value };
	if (this->pair_count == 0) {
		this->pairs[0] = new_pair;
		this->pair_count++;
		return true;
	}

	uint64_t index = UINT64_MAX;
	if (lookup_index(this, key, &index)) {
		log_verbose(1, "key %" PRIu64 " already present "
				"when inserting %" PRIu64 "=%" PRIu64,
				key, key, value);
		return false;
	}
	assert(index != UINT64_MAX);
	if (this->pairs[index].key < key) {
		memmove(this->pairs + index + 2, this->pairs + index + 1,
				sizeof(pair) * (this->pair_count - (index + 1)));
		this->pairs[index + 1] = new_pair;
	} else {
		ASSERT(index == 0);
		memmove(this->pairs + 1, this->pairs, this->pair_count * sizeof(pair));
		this->pairs[0] = new_pair;
	}
	++this->pair_count;
	return true;
}

static bool delete(void* _this, uint64_t key) {
	data* this = _this;

	uint64_t index;
	if (!lookup_index(this, key, &index)) {
		// ERROR: no such element
		return false;
	}
	memmove(&this->pairs[index], &this->pairs[index + 1],
			sizeof(pair) * (this->pair_count - index - 1));
	this->pair_count--;
	return true;
}

const dict_api dict_array = {
	.init = init,
	.destroy = destroy,

	.insert = insert,
	.find = find,
	.delete = delete,

	.next = next,
	.prev = prev,

	.name = "dict_array"
};
