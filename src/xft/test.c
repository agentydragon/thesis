#include "xft/test.h"

#include <assert.h>
#include <stdlib.h>

#include "dict/dict.h"
#include "log/log.h"
#include "xft/xft.h"

static void test_init_destroy() {
	xft trie;
	xft_init(&trie);
	xft_destroy(&trie);
}

static void test_contains() {
	xft trie;
	xft_init(&trie);

	xft_node node = {
		.prefix = 0xDEADBEEFDEADBEEF
	};

	assert(!dict_insert(trie.lss[64], 0xDEADBEEFDEADBEEF,
			(uint64_t) &node));

	assert(xft_contains(&trie, 0xDEADBEEFDEADBEEF));
	assert(!xft_contains(&trie, 0x0BADF00D0BADF00D));
	xft_destroy(&trie);
}

static void test_prefix() {
	assert(xft_prefix(0xDEADBEEF0BADF00D, 0) == 0);
	assert(xft_prefix(0xDEADBEEF0BADF00D, 15) == 0xDEAC000000000000);
	assert(xft_prefix(0xDEADBEEF0BADF00D, 17) == 0xDEAD800000000000);
	assert(xft_prefix(0xDEADBEEF0BADF00D, 64) == 0xDEADBEEF0BADF00D);
}

void test_xft() {
	test_init_destroy();
	test_contains();
	test_prefix();
	log_info("xft ok");
}
