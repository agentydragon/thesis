#include "dict/rbtree.h"

#include "log/log.h"

typedef struct {
	uint64_t key;
	uint64_t value;
} rbtree_pair;

// Declares some libucw functions, like xmalloc/xfree.
#include <ucw/lib.h>

#define TREE_NODE rbtree_pair
#define TREE_PREFIX(x) rbtree_##x
#define TREE_KEY_ATOMIC key

#define TREE_WANT_CLEANUP
#define TREE_WANT_FIND
//#define TREE_WANT_SEARCH_DOWN
//#define TREE_WANT_SEARCH_UP
#define TREE_WANT_DELETE
#define TREE_WANT_NEW

#define TREE_ATOMIC_TYPE uint64_t

#include <ucw/redblack.h>

static void init(void** _this) {
	struct rbtree_tree* tree = malloc(sizeof(struct rbtree_tree));
	CHECK(tree, "cannot allocate rbtree");
	rbtree_init(tree);
	*_this = tree;
}

static void destroy(void** _this) {
	if (_this) {
		struct rbtree_tree* tree = *_this;
		rbtree_cleanup(tree);
		free(tree);
		*_this = NULL;
	}
}

static bool insert(void* _this, uint64_t key, uint64_t value) {
	struct rbtree_tree* this = _this;
	// TODO: Probably suboptimal, could maybe combine find with insert.
	if (rbtree_find(this, key)) {
		return false;
	}
	rbtree_pair* pair = rbtree_new(this, key);
	pair->value = value;
	return true;
}

static bool find(void* _this, uint64_t key, uint64_t *value) {
	struct rbtree_tree* this = _this;
	rbtree_pair* pair = rbtree_find(this, key);
	if (pair) {
		*value = pair->value;
		return true;
	} else {
		return false;
	}
}

static bool delete(void* _this, uint64_t key) {
	struct rbtree_tree* this = _this;
	return rbtree_delete(this, key);
}

// TODO: next, prev

const dict_api dict_rbtree = {
	.init = init,
	.destroy = destroy,

	.insert = insert,
	.find = find,
	.delete = delete,

	// .next = next,
	// .prev = prev,

	.name = "dict_rbtree"
};
