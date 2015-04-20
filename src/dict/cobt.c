#include "dict/cobt.h"

#include "log/log.h"
#include "cobt/cobt.h"

#include <stdlib.h>
#include <string.h>

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	cob* this = malloc(sizeof(cob));
	if (!this) return 1;

	memset(this, 0, sizeof(cob));
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

static bool find(void* this, uint64_t key, uint64_t *value) {
	return cob_find(this, key, value);
}

static int8_t insert(void* this, uint64_t key, uint64_t value) {
	return cob_insert(this, key, value);
}

static int8_t delete(void* this, uint64_t key) {
	return cob_delete(this, key);
}

static bool next(void* this, uint64_t key, uint64_t *next_key) {
	return cob_next_key(this, key, next_key);
}

static bool prev(void* this, uint64_t key, uint64_t *prev_key) {
	return cob_previous_key(this, key, prev_key);
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
