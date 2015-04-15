#include "dict/btree.h"

#include <assert.h>
#include <stdlib.h>

#include "btree/btree.h"

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	btree* this = malloc(sizeof(btree));
	assert(this);
	btree_init(this);
	*_this = this;

	return 0;
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

static void find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	btree_find(_this, key, value, found);
}

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	return btree_insert(_this, key, value);
}

static int8_t delete(void* _this, uint64_t key) {
	return btree_delete(_this, key);
}

const dict_api dict_btree = {
	.init = init,
	.destroy = destroy,

	.find = find,
	.insert = insert,
	.delete = delete,

	// TODO: next, prev

	.dump = NULL,
	.name = "dict_btree"
};
