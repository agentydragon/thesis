#include "test.h"
#include "cache_oblivious_btree.h"
#include "../log/log.h"
#include "../math/math.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../veb_layout/veb_layout.h"
#include "../test/ordered_hash_blackbox.h"

static void blackbox_init(void** _this) {
	*_this = malloc(sizeof(struct cob));
	cob_init(*_this);
}

static void blackbox_destroy(void** _this) {
	cob_destroy(** (struct cob**) _this);
	free(*_this);
	*_this = NULL;
}

static void blackbox_check(void* _this) {
	struct cob* cob = _this;
	// cob_dump(*cob);
	for (uint64_t i = 0; i < cob->file.capacity / cob->file.block_size;
			i++) {
		const uint64_t leaf_offset = i * cob->file.block_size;
		const uint64_t node = veb_get_leaf_number(i, cobt_get_veb_height(*cob));
		assert(cob->veb_minima[node] == cobt_range_get_minimum((ofm_range) {
				.begin = leaf_offset,
				.size = cob->file.block_size,
				.file = &cob->file
		}));
	}
}

static ordered_hash_blackbox_spec blackbox_api = {
	.init = blackbox_init,
	.destroy = blackbox_destroy,
	.insert = (void(*)(void*, uint64_t, uint64_t)) cob_insert,
	.remove = (void(*)(void*, uint64_t)) cob_delete,
	.find = (void(*)(void*, uint64_t, bool*, uint64_t*)) cob_find,
	.next_key = (void(*)(void*, uint64_t, bool*, uint64_t*)) cob_next_key,
	.previous_key = (void(*)(void*, uint64_t, bool*, uint64_t*)) cob_previous_key,
	.check = blackbox_check
};

void test_cache_oblivious_btree() {
	test_ordered_hash_blackbox(blackbox_api);
}
