#include "observation.h"
#include "../log/log.h"

#include <stdlib.h>
#include <assert.h>
#include <inttypes.h> // PRIu64

const uint64_t MIN_CAPACITY = 4;

struct observation {
	struct observed_operation* operations;
	uint64_t size, capacity;
};

int8_t observation_init(observation** _this) {
	observation* this = malloc(sizeof(observation));
	if (!this) {
		log_error("cannot allocate observation");
		goto err_1;
	}
	*this = (observation) {
		.operations = malloc(sizeof(struct observed_operation) * MIN_CAPACITY),
		.size = 0,
		.capacity = MIN_CAPACITY
	};
	if (!this->operations) {
		log_error("cannot allocate observed operations buffer");
		goto err_2;
	}
	*_this = this;
	return 0;

err_2:
	free(this);
err_1:
	return 1;
}

void observation_destroy(observation** _this) {
	if (_this) {
		observation* this = *_this;
		free(this);
		*_this = NULL;
	}
}

static int8_t observation_push(observation* this, struct observed_operation observed) {
	if (this->size == this->capacity) {
		assert(this->capacity);
		uint64_t new_capacity = this->capacity * 2;
		struct observed_operation* new_buffer = realloc(this->operations, sizeof(struct observed_operation) * new_capacity);

		if (!new_buffer) {
			log_error("cannot expand space for more observed operations");
			return 1;
		}

		this->capacity = new_capacity;
		this->operations = new_buffer;
	}

	this->operations[this->size++] = observed;
	return 0;
}

struct tapped_init_args {
	observation* recorded_observation;
	hash* hash_to_tap;
};

struct tapped_hash {
	observation* recorded_observation;
	hash* hash_to_tap;
};

static int8_t tapped_init(void** _this, void* _args) {
	struct tapped_init_args* args = _args;
	struct tapped_hash* this = malloc(sizeof(struct tapped_hash));
	if (!this) {
		log_error("cannot allocate tapped hash");
		return 1;
	}
	*this = (struct tapped_hash) {
		.recorded_observation = args->recorded_observation,
		.hash_to_tap = args->hash_to_tap
	};
	*_this = this;
	return 0;
}

static void tapped_destroy(void** _this) {
	if (_this) {
		struct tapped_hash *this = *_this;
		free(this);
		*_this = NULL;
	}
}

static int8_t tapped_find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	struct tapped_hash *this = _this;

	if (hash_find(this->hash_to_tap, key, value, found)) {
		log_error("tapped hash_find failed");
		return 1;
	}

	if (observation_push(this->recorded_observation, (struct observed_operation) {
		.operation = OP_FIND,
		.find = { .key = key }
	})) {
		log_fatal("cannot push hash_find observation");
		return 1;
	}
	return 0;
}

static int8_t tapped_insert(void* _this, uint64_t key, uint64_t value) {
	struct tapped_hash *this = _this;

	if (hash_insert(this->hash_to_tap, key, value)) {
		log_error("tapped hash_insert failed");
		return 1;
	}

	if (observation_push(this->recorded_observation, (struct observed_operation) {
		.operation = OP_INSERT,
		.insert = { .key = key, .value = value }
	})) {
		log_fatal("cannot push hash_insert observation");
		return 1;
	}
	return 0;
}

static int8_t tapped_delete(void* _this, uint64_t key) {
	struct tapped_hash *this = _this;

	if (hash_delete(this->hash_to_tap, key)) {
		log_error("tapped hash_delete failed");
		return 1;
	}

	if (observation_push(this->recorded_observation, (struct observed_operation) {
		.operation = OP_DELETE,
		.delete = { .key = key }
	})) {
		log_fatal("cannot push hash_delete observation");
		return 1;
	}
	return 0;
}

hash_api tapped_hash_api = {
	.init = tapped_init,
	.destroy = tapped_destroy,

	.find = tapped_find,
	.insert = tapped_insert,
	.delete = tapped_delete,

	.name = "tapped_hash"
};

int8_t observation_tap(observation* this, hash* to_tap, hash** tapped) {
	return hash_init(tapped, &tapped_hash_api, & (struct tapped_init_args) {
		.recorded_observation = this,
		.hash_to_tap = to_tap
	});
}

static int8_t replay_operation(struct observed_operation operation, hash* replay_on) {
	switch (operation.operation) {
		case OP_FIND:
			return hash_find(replay_on, operation.find.key, NULL, NULL);
		case OP_INSERT:
			return hash_insert(replay_on, operation.insert.key, operation.insert.value);
		case OP_DELETE:
			return hash_delete(replay_on, operation.delete.key);
		default:
			log_fatal("unknown operation type %d", operation.operation);
			return 1;
	}
}

int8_t observation_replay(observation* this, hash* replay_on) {
	for (uint64_t i = 0; i < this->size; i++) {
		if (replay_operation(this->operations[i], replay_on)) {
			log_fatal("failed to replay operation %" PRIu64 " of %" PRIu64, i, this->size);
			return 1;
		}
	}
	return 0;
}

/*
int8_t observation_load(observation* this, const char* filename) {
	log_fatal("observation_load not implemented");
	return 1;
}

int8_t observation_save(observation* this, const char* filename) {
	log_fatal("observation_save not implemented");
	return 1;
}
*/
