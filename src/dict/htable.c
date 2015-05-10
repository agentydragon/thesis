#include "dict/htable.h"

#include <stdlib.h>

#include "htable/htable.h"
#include "log/log.h"

static void init(void** _this) {
	htable* this = malloc(sizeof(htable));
	CHECK(this, "cannot allocate new htable");
	htable_init(this);
	*_this = this;
}

static void destroy(void** _this) {
	if (_this) {
		htable* this = *_this;
		htable_destroy(this);
		free(this);
		*_this = NULL;
	}
}

static bool insert(void* this, uint64_t key, uint64_t value) {
	return htable_insert(this, key, value);
}

static bool delete(void* this, uint64_t key) {
	return htable_delete(this, key);
}

static bool find(void* this, uint64_t key, uint64_t* value) {
	return htable_find(this, key, value);
}

const dict_api dict_htable = {
	.init = init,
	.destroy = destroy,

	.find = find,
	.insert = insert,
	.delete = delete,

	.name = "dict_htable"
};
