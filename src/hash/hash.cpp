#include "hash.h"

#include <stdlib.h>

#include "../log/log.h"

struct hash_s {
	void* opaque;
	const hash_api* api;
};

int8_t hash_init(hash** _this, const hash_api* api, void* args) {
	hash* this = malloc(sizeof(struct hash_s));

	if (!this) {
		goto err_1;
	}

	this->api = api;
	if (this->api->init(&this->opaque, args)) {
		goto err_2;
	}

	*_this = this;
	return 0;

err_2:
	free(this);
err_1:
	return 1;
}

void hash_destroy(hash** _this) {
	if (*_this) {
		hash* this = *_this;
		this->api->destroy(&this->opaque);
		free(this);
		*_this = NULL;
	}
}

int8_t hash_find(hash* this, uint64_t key, uint64_t *value, bool *found) {
	return this->api->find(this->opaque, key, value, found);
}

int8_t hash_insert(hash* this, uint64_t key, uint64_t value) {
	return this->api->insert(this->opaque, key, value);
}

int8_t hash_remove(hash* this, uint64_t key) {
	return this->api->remove(this->opaque, key);
}

void hash_dump(hash* this) {
	if (this->api->dump) {
		this->api->dump(this->opaque);
	} else {
		log_info("cannot dump hash of type %s", this->api->name);
	}
}
