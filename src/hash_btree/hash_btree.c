#include "hash_btree.h"
#include "../btree/btree.h"

#include <assert.h>
#include <stdlib.h>

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

static int8_t find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	btree_find(_this, key, found, value);
	return 0;
}

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	btree_insert(_this, key, value);
	return 0;
}

const hash_api hash_btree = {
	.init = init,
	.destroy = destroy,

	.find = find,
	.insert = insert,
	.delete = btree_delete,

	.dump = NULL,
	.name = "hash_btree"
};
