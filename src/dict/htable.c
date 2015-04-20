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

	*this = (htable) {
		.blocks = NULL,
		.blocks_size = 0,
		.pair_count = 0
	};
	*_this = this;
	return 0;
}

static void destroy(void** _this) {
	if (_this) {
		htable* this = *_this;
		if (this) {
			free(this->blocks);
		}
		free(this);
		*_this = NULL;
	}
}

static int8_t insert(void* _this, uint64_t key, uint64_t value) {
	htable* this = _this;
	log_info("insert(%" PRIx64 ", %" PRIx64 ")", key, value);
	return htable_insert(this, key, value);
}

static int8_t delete(void* _this, uint64_t key) {
	htable* this = _this;
	return htable_delete(this, key);
}

const dict_api dict_htable = {
	.init = init,
	.destroy = destroy,

	.find = htable_find,
	.insert = insert,
	.delete = delete,

	.name = "dict_htable"
};
