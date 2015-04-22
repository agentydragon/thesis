#include "dict/htcuckoo.h"

#include <stdlib.h>

#include "htable/cuckoo.h"
#include "rand/rand.h"
#include "log/log.h"
#include "util/unused.h"

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	htcuckoo* this = malloc(sizeof(htcuckoo));
	if (!this) {
		log_error("cannot allocate new htcuckoo");
		return 1;
	}
	rand_generator rand = { .state = 0 };
	// rand_seed_with_time(&rand);
	htcuckoo_init(this, rand);
	*_this = this;
	return 0;
}

static void destroy(void** _this) {
	if (_this) {
		htcuckoo* this = *_this;
		htcuckoo_destroy(this);
		free(this);
		*_this = NULL;
	}
}

static int8_t insert(void* this, uint64_t key, uint64_t value) {
	return htcuckoo_insert(this, key, value);
}

static int8_t delete(void* this, uint64_t key) {
	return htcuckoo_delete(this, key);
}

static bool find(void* this, uint64_t key, uint64_t* value) {
	return htcuckoo_find(this, key, value);
}

const dict_api dict_htcuckoo = {
	.init = init,
	.destroy = destroy,

	.find = find,
	.insert = insert,
	.delete = delete,

	.name = "dict_htcuckoo"
};
