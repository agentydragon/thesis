#include "dict/btree.h"

#include <assert.h>
#include <stdlib.h>

#include "btree/btree.h"

static void init(void** _this) {
	btree* this = malloc(sizeof(btree));
	assert(this);
	btree_init(this);
	*_this = this;
}

static void destroy(void** _this) {
	if (_this) {
		btree* this = *_this;
		if (this) {
			btree_destroy(this);
			free(this);
		}
		*_this = NULL;
	}
}

static bool find(void* this, uint64_t key, uint64_t *value) {
	return btree_find(this, key, value);
}

static bool next(void* this, uint64_t key, uint64_t *next_key) {
	return btree_find_next(this, key, next_key);
}

static bool prev(void* this, uint64_t key, uint64_t *prev_key) {
	return btree_find_prev(this, key, prev_key);
}

static bool insert(void* this, uint64_t key, uint64_t value) {
	return btree_insert(this, key, value);
}

static bool delete(void* this, uint64_t key) {
	return btree_delete(this, key);
}

const dict_api dict_btree = {
	.init = init,
	.destroy = destroy,

	.find = find,
	.insert = insert,
	.delete = delete,

	.next = next,
	.prev = prev,

	.dump = NULL,
	.name = "dict_btree"
};
