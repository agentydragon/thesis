#include "hash_cobt.h"

#include "../log/log.h"
#include "../cache_oblivious_btree/cache_oblivious_btree.h"

#include <stdlib.h>

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	struct cob* cob = malloc(sizeof(struct cob));
	if (!cob) return 1;

	cob_init(cob);
	*_this = cob;

	return 0;
}

static void destroy(void** _this) {
	if (_this) {
		struct cob* cob = * (struct cob**) _this;
		cob_destroy(*cob);
		free(cob);
		*_this = NULL;
	}
}

static int8_t find(void* _this, uint64_t key, uint64_t *value, bool *found) {
	cob_find(_this, key, found, value);  // TODO: fix arg order
	return 0;
}

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	cob_insert(_this, key, value);
	return 0;
}

static int8_t delete(void* _this, uint64_t key) {
	// TODO: error on element not found
	return cob_delete(_this, key);
}

const hash_api hash_cobt = {
	.init = init,
	.destroy = destroy,

	.insert = insert,
	.find = find,
	.delete = delete,

	.name = "hash_cobt"
};
