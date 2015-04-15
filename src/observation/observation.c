#include "observation/observation.h"

#include <stdlib.h>
#include <assert.h>
#include <inttypes.h> // PRIu64

#include "log/log.h"

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
		.operations = calloc(MIN_CAPACITY, sizeof(struct observed_operation)),
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
		free(this->operations);
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
	dict* dict_to_tap;
};

struct tapped_dict {
	observation* recorded_observation;
	dict* dict_to_tap;
};

static int8_t tapped_init(void** _this, void* _args) {
	struct tapped_init_args* args = _args;
	struct tapped_dict* this = malloc(sizeof(struct tapped_dict));
	if (!this) {
		log_error("cannot allocate tapped dict");
		return 1;
	}
	*this = (struct tapped_dict) {
		.recorded_observation = args->recorded_observation,
		.dict_to_tap = args->dict_to_tap
	};
	*_this = this;
	return 0;
}

static void tapped_destroy(void** _this) {
	if (_this) {
		struct tapped_dict *this = *_this;
		free(this);
		*_this = NULL;
	}
}

static void tapped_find(void* _this, uint64_t key,
		uint64_t *value, bool *found) {
	struct tapped_dict *this = _this;

	dict_find(this->dict_to_tap, key, value, found);

	ASSERT(!observation_push(this->recorded_observation,
				(struct observed_operation) {
					.operation = OP_FIND,
					.find = { .key = key }
				}));
}

static int8_t tapped_insert(void* _this, uint64_t key, uint64_t value) {
	struct tapped_dict *this = _this;

	if (dict_insert(this->dict_to_tap, key, value)) {
		log_error("tapped dict_insert failed");
		return 1;
	}

	if (observation_push(this->recorded_observation,
				(struct observed_operation) {
					.operation = OP_INSERT,
					.insert = { .key = key, .value = value }
				})) {
		log_fatal("cannot push dict_insert observation");
		return 1;
	}
	return 0;
}

static int8_t tapped_delete(void* _this, uint64_t key) {
	struct tapped_dict *this = _this;

	if (dict_delete(this->dict_to_tap, key)) {
		log_error("tapped dict_delete failed");
		return 1;
	}

	if (observation_push(this->recorded_observation, (struct observed_operation) {
		.operation = OP_DELETE,
		.delete = { .key = key }
	})) {
		log_fatal("cannot push dict_delete observation");
		return 1;
	}
	return 0;
}

dict_api tapped_dict_api = {
	.init = tapped_init,
	.destroy = tapped_destroy,

	.find = tapped_find,
	.insert = tapped_insert,
	.delete = tapped_delete,

	.name = "tapped_dict"
};

int8_t observation_tap(observation* this, dict* to_tap, dict** tapped) {
	return dict_init(tapped, &tapped_dict_api, & (struct tapped_init_args) {
		.recorded_observation = this,
		.dict_to_tap = to_tap
	});
}

static int8_t replay_operation(struct observed_operation operation, dict* replay_on) {
	switch (operation.operation) {
		case OP_FIND:
			dict_find(replay_on, operation.find.key, NULL, NULL);
			return 0;
		case OP_INSERT:
			return dict_insert(replay_on, operation.insert.key, operation.insert.value);
		case OP_DELETE:
			return dict_delete(replay_on, operation.delete.key);
		default:
			log_fatal("unknown operation type %d", operation.operation);
			return 1;
	}
}

int8_t observation_replay(observation* this, dict* replay_on) {
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
