#include "dict/ksplay.h"

#include <assert.h>
#include <stdlib.h>

#include "ksplay/ksplay.h"

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;
	ksplay* this = malloc(sizeof(ksplay));
	if (!this) {
		return 1;
	}
	ksplay_init(this);
	*_this = this;
	return 0;
}

static void destroy(void** _this) {
	if (_this) {
		ksplay_destroy(* (ksplay**) _this);
		free(*_this);
		*_this = NULL;
	}
}

static int8_t find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	assert(_this);
	ksplay_find(_this, key, value, found);
	return 0;
}

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	assert(_this);
	return ksplay_insert(_this, key, value);
}

static int8_t delete(void* _this, uint64_t key) {
	assert(_this);
	return ksplay_delete(_this, key);
}

const dict_api dict_ksplay = {
	.init = init,
	.destroy = destroy,

	.insert = insert,
	.find = find,
	.delete = delete,

	// TODO: next, prev
	.next = NULL,
	.prev = NULL,

	.name = "dict_ksplay"
};
