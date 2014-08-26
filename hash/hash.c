#include "hash.h"

#include <stdlib.h>

struct hash_s {
	void* opaque;
	const hash_api* api;
};

int8_t hash_init(hash** _this, const hash_api* api) {
	hash* this = malloc(sizeof(struct hash_s));

	if (!this) {
		goto err_1;
	}

	this->api = api;
	if (this->api->init(&this->opaque)) {
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

int8_t hash_delete(hash* this, uint64_t key) {
	return this->api->delete(this->opaque, key);
}
