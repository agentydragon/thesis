#include "dict/cobt.h"

#include "log/log.h"
#include "cobt/cobt.h"

#include <stdlib.h>

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	cob* this = malloc(sizeof(cob));
	if (!this) return 1;

	cob_init(this);
	*_this = this;

	return 0;
}

static void destroy(void** _this) {
	if (_this) {
		cob* this = * (cob**) _this;
		cob_destroy(this);
		free(this);
		*_this = NULL;
	}
}

static int8_t find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	cob_find(_this, key, found, value);  // TODO: fix arg order
	return 0;
}

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	return cob_insert(_this, key, value);
}

static int8_t delete(void* _this, uint64_t key) {
	return cob_delete(_this, key);
}

static int8_t next(void* _this, uint64_t key, uint64_t *next_key, bool *found) {
	cob_next_key(_this, key, found, next_key);
	return 0;
}

static int8_t prev(void* _this, uint64_t key, uint64_t *prev_key, bool *found) {
	cob_previous_key(_this, key, found, prev_key);
	return 0;
}

const dict_api dict_cobt = {
	.init = init,
	.destroy = destroy,

	.insert = insert,
	.find = find,
	.delete = delete,

	.next = next,
	.prev = prev,

	.name = "dict_cobt"
};
