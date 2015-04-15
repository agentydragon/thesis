#include "dict/kforest.h"

#include <assert.h>
#include <stdlib.h>

#include "kforest/kforest.h"

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;
	kforest* this = malloc(sizeof(kforest));
	if (!this) {
		goto err_1;
		return 1;
	}
	if (kforest_init(this)) {
		goto err_2;
	}
	*_this = this;
	return 0;

err_2:
	free(this);
err_1:
	return 1;
}

static void destroy(void** _this) {
	if (_this) {
		kforest_destroy(* (kforest**) _this);
		free(*_this);
		*_this = NULL;
	}
}

static void find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	assert(_this);
	kforest_find(_this, key, value, found);
}

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	assert(_this);
	return kforest_insert(_this, key, value);
}

static int8_t delete(void* _this, uint64_t key) {
	assert(_this);
	return kforest_delete(_this, key);
}

const dict_api dict_kforest = {
	.init = init,
	.destroy = destroy,

	.insert = insert,
	.find = find,
	.delete = delete,

	.next = NULL,
	.prev = NULL,

	.name = "dict_kforest"
};
