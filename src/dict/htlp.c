#include "dict/htlp.h"

#include <stdlib.h>

#include "htable/open.h"
#include "rand/rand.h"
#include "log/log.h"

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	htlp* this = malloc(sizeof(htlp));
	if (!this) {
		log_error("cannot allocate new htlp");
		return 1;
	}
	rand_generator rand;
	rand_seed_with_time(&rand);
	htlp_init(this, rand);
	*_this = this;
	return 0;
}

static void destroy(void** _this) {
	if (_this) {
		htlp* this = *_this;
		htlp_destroy(this);
		free(this);
		*_this = NULL;
	}
}

static int8_t insert(void* this, uint64_t key, uint64_t value) {
	return htlp_insert(this, key, value);
}

static int8_t delete(void* this, uint64_t key) {
	return htlp_delete(this, key);
}

static bool find(void* this, uint64_t key, uint64_t* value) {
	return htlp_find(this, key, value);
}

const dict_api dict_htlp = {
	.init = init,
	.destroy = destroy,

	.find = find,
	.insert = insert,
	.delete = delete,

	.name = "dict_htlp"
};
