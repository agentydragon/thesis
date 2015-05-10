#include "dict/ksplay.h"

#include <assert.h>
#include <stdlib.h>

#include "ksplay/ksplay.h"
#include "log/log.h"

static void init(void** _this) {
	ksplay* this = malloc(sizeof(ksplay));
	CHECK(this, "failed to allocate space for ksplay");
	ksplay_init(this);
	*_this = this;
}

static void destroy(void** _this) {
	if (_this) {
		ksplay_destroy(* (ksplay**) _this);
		free(*_this);
		*_this = NULL;
	}
}

static bool find(void* this, uint64_t key, uint64_t *value) {
	return ksplay_find(this, key, value);
}

static bool find_next(void* this, uint64_t key, uint64_t* next_key) {
	return ksplay_next_key(this, key, next_key);
}

static bool find_previous(void* this, uint64_t key, uint64_t* previous_key) {
	return ksplay_previous_key(this, key, previous_key);
}

static bool insert(void* this, uint64_t key, uint64_t value) {
	return ksplay_insert(this, key, value);
}

static bool delete(void* this, uint64_t key) {
	return ksplay_delete(this, key);
}

const dict_api dict_ksplay = {
	.init = init,
	.destroy = destroy,

	.insert = insert,
	.find = find,
	.delete = delete,

	.next = find_next,
	.prev = find_previous,

	.name = "dict_ksplay"
};
