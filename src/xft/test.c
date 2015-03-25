#include "xft/test.h"

#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>

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

static void insert_all_prefixes(xft* trie, uint64_t key) {
	for (uint64_t prefix_length = 1; prefix_length <= 64; ++prefix_length) {
		const uint64_t prefix = xft_prefix(key, prefix_length);
		xft_node* node = malloc(sizeof(xft_node));
		node->prefix = prefix;
		log_info("insert %p: %" PRIx64, node, prefix);
		assert(!dict_insert(trie->lss[prefix_length], prefix,
					(uint64_t) &node));
	}
}

static void test_lowest_ancestor() {
	xft_node* prefix_node;

	xft trie;
	xft_init(&trie);

	insert_all_prefixes(&trie, 0xDEADBEEF0BADF00D);
	prefix_node = xft_lowest_ancestor(&trie, 0x0123456789ABCDEF);
	log_info("%p: %" PRIx64, prefix_node, prefix_node->prefix);
	assert(prefix_node->prefix == 0x0000000000000000);

	prefix_node = xft_lowest_ancestor(&trie, 0xDEADBEEF0BADF00D);
	assert(prefix_node->prefix == 0xDEADBEEF0BADF00D);

	prefix_node = xft_lowest_ancestor(&trie, 0xDEADBEEF0B999999);
	assert(prefix_node->prefix == 0xDEADBEEF0B000000);

	for (uint64_t prefix_length = 0; prefix_length <= 64; ++prefix_length) {
		const uint64_t prefix = xft_prefix(0xDEADBEEF0BADF00D,
				prefix_length);
		bool found;
		assert(!dict_find(trie.lss[prefix_length], prefix,
					(uint64_t*) &prefix_node, &found));
		assert(found);
		free(prefix_node);
	}
	xft_destroy(&trie);
}

void test_xft() {
	test_init_destroy();
	test_contains();
	test_lowest_ancestor();
	test_prefix();
	log_info("xft ok");
}
