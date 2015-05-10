#include "dict/dict.h"

#include <stdlib.h>

#include "log/log.h"

struct dict_s {
	void* opaque;
	const dict_api* api;
};

void dict_init(dict** _this, const dict_api* api) {
	dict* this = malloc(sizeof(struct dict_s));
	CHECK(this, "failed to allocate memory for %s", api->name);
	this->api = api;
	this->api->init(&this->opaque);
	*_this = this;
}

void dict_destroy(dict** _this) {
	if (*_this) {
		dict* this = *_this;
		this->api->destroy(&this->opaque);
		free(this);
		*_this = NULL;
	}
}

bool dict_find(dict* this, uint64_t key, uint64_t *value) {
	return this->api->find(this->opaque, key, value);
}

bool dict_insert(dict* this, uint64_t key, uint64_t value) {
	return this->api->insert(this->opaque, key, value);
}

bool dict_delete(dict* this, uint64_t key) {
	return this->api->delete(this->opaque, key);
}

void dict_dump(dict* this) {
	if (this->api->dump) {
		this->api->dump(this->opaque);
	} else {
		log_info("cannot dump dict of type %s", this->api->name);
	}
}

void dict_check(dict* this) {
	if (this->api->check) {
		this->api->check(this->opaque);
	}
}

bool dict_api_allows_order_queries(const dict_api* api) {
	return api->next && api->prev;
}

bool dict_allows_order_queries(const dict* this) {
	return dict_api_allows_order_queries(this->api);
}

bool dict_next(dict* this, uint64_t key, uint64_t *next_key) {
	if (this->api->next) {
		return this->api->next(this->opaque, key, next_key);
	}
	log_fatal("dict api %s doesn't implement next", this->api->name);
}

bool dict_prev(dict* this, uint64_t key, uint64_t *prev_key) {
	if (this->api->prev) {
		return this->api->prev(this->opaque, key, prev_key);
	}
	log_fatal("dict api %s doesn't implement prev", this->api->name);
}

const void* dict_get_implementation(dict* this) {
	return this->opaque;
}

const dict_api* dict_get_api(dict* this) {
	return this->api;
}

bool dict_contains(dict* this, uint64_t key) {
	return dict_find(this, key, NULL);
}
