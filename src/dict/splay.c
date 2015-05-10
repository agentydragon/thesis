#include "dict/splay.h"

#include <assert.h>
#include <stdlib.h>

#include "log/log.h"
#include "splay/splay.h"

static void init(void** _this) {
	splay_tree* this = malloc(sizeof(splay_tree));
	CHECK(this, "failed to allocate memory for splay");
	this->root = NULL;
	*_this = this;
}

static void destroy(void** _this) {
	if (_this) {
		splay_destroy((splay_tree **) _this);
	}
}

static bool find(void* this, uint64_t key, uint64_t *value) {
	return splay_find(this, key, value);
}

static bool insert(void* this, uint64_t key, uint64_t value) {
	return splay_insert(this, key, value);
}

static bool delete(void* this, uint64_t key) {
	return splay_delete(this, key);
}

static bool next(void* this, uint64_t key, uint64_t *next_key) {
	return splay_next_key(this, key, next_key);
}

static bool prev(void* this, uint64_t key, uint64_t *previous_key) {
	return splay_previous_key(this, key, previous_key);
}

const dict_api dict_splay = {
	.init = init,
	.destroy = destroy,

	.insert = insert,
	.find = find,
	.delete = delete,

	.next = next,
	.prev = prev,

	.name = "dict_splay"
};
