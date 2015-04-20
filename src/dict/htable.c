#include "dict/htable.h"

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#define NO_LOG_INFO

#include "log/log.h"

#include "htable/htable.h"

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	htable* this = malloc(sizeof(htable));
	if (!this) {
		log_error("cannot allocate new htable");
		return 1;
	}

	htable_init(this);
	*_this = this;
	return 0;
}

static void destroy(void** _this) {
	if (_this) {
		htable* this = *_this;
		htable_destroy(this);
		free(this);
		*_this = NULL;
	}
}

static int8_t insert(void* this, uint64_t key, uint64_t value) {
	return htable_insert(this, key, value);
}

static int8_t delete(void* this, uint64_t key) {
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
