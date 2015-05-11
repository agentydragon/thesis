#include "dict/register.h"
#include "log/log.h"

#include <inttypes.h>
#include <string.h>

#include "dict/array.h"
#include "dict/btree.h"
#include "dict/cobt.h"
#include "dict/htcuckoo.h"
#include "dict/htlp.h"
#include "dict/kforest.h"
#include "dict/ksplay.h"
#include "dict/rbtree.h"
#include "dict/splay.h"

const dict_api* DICT_API_REGISTER[] = {
	&dict_array, &dict_btree, &dict_cobt,
	&dict_htlp, &dict_htcuckoo,
	&dict_kforest, &dict_ksplay, &dict_splay,
	&dict_rbtree,
	NULL
};

const dict_api* dict_api_find(const char* name) {
	for (const dict_api** api = &DICT_API_REGISTER[0]; *api; ++api) {
		if (strcmp((*api)->name, name) == 0) {
			return *api;
		}
	}
	return NULL;
}

void dict_api_list_parse(char* name_list, dict_api const ** list,
		uint64_t capacity) {
	const char comma[] = ",";
	char* token = strtok(name_list, comma);
	uint64_t i;
	for (i = 0; token != NULL; i++, token = strtok(NULL, comma)) {
		const dict_api* found = dict_api_find(token);
		CHECK(found != NULL, "no such dict api: %s", token);
		CHECK(i < capacity, "too many (>=%" PRIu64 ") APIs to measure",
				capacity);
		list[i] = found;
	}
	list[i] = NULL;
}
