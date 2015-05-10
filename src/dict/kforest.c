#include "dict/kforest.h"

#include <assert.h>
#include <stdlib.h>

#include "kforest/kforest.h"
#include "log/log.h"

static void init(void** _this) {
	kforest* this = malloc(sizeof(kforest));
	CHECK(this, "failed to allocate memory for kforest");
	CHECK(!kforest_init(this), "failed to init kforest");
	*_this = this;
}

static void destroy(void** _this) {
	if (_this) {
		kforest_destroy(* (kforest**) _this);
		free(*_this);
		*_this = NULL;
	}
}

static bool find(void* _this, uint64_t key, uint64_t *value) {
	return kforest_find(_this, key, value);
}

static bool insert(void* _this, uint64_t key, uint64_t value) {
	return kforest_insert(_this, key, value);
}

static bool delete(void* _this, uint64_t key) {
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
