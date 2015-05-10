#include "dict/htcuckoo.h"

#include <stdlib.h>

#include "htable/cuckoo.h"
#include "rand/rand.h"
#include "log/log.h"
#include "util/unused.h"

static void init(void** _this) {
	htcuckoo* this = malloc(sizeof(htcuckoo));
	CHECK(this, "cannot allocate new htcuckoo");
	rand_generator rand = { .state = 0 };
	// rand_seed_with_time(&rand);
	htcuckoo_init(this, rand);
	*_this = this;
}

static void destroy(void** _this) {
	if (_this) {
		htcuckoo* this = *_this;
		htcuckoo_destroy(this);
		free(this);
		*_this = NULL;
	}
}

static bool insert(void* this, uint64_t key, uint64_t value) {
	return htcuckoo_insert(this, key, value);
}

static bool delete(void* this, uint64_t key) {
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
