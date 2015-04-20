#include "dict/splay.h"

#include <assert.h>
#include <stdlib.h>

#include "splay/splay.h"

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;
	splay_tree* this = malloc(sizeof(splay_tree));
	if (!this) {
		return 1;
	}
	this->root = NULL;
	*_this = this;
	return 0;
}

static void destroy(void** _this) {
	if (_this) {
		splay_destroy((splay_tree **) _this);
	}
}

static bool find(void* this, uint64_t key, uint64_t *value) {
	return splay_find(this, key, value);
}

static int8_t insert(void* this, uint64_t key, uint64_t value) {
	return splay_insert(this, key, value);
}

static int8_t delete(void* this, uint64_t key) {
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
