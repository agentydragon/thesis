#include "hash_bplustree.h"
#include "private/find.h"
#include "private/insert.h"
#include "private/delete.h"
#include "private/dump.h"
#include "private/data.h"

#include "../log/log.h"

#include <stdlib.h>

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	struct hashbplustree* this = malloc(sizeof(struct hashbplustree));
	if (!this) {
		goto err_1;
	}

	this->root = malloc(sizeof(struct hashbplustree_node));
	if (!this->root) {
		goto err_2;
	}

	this->root->is_leaf = true;
	this->root->keys_count = 0;

	*_this = this;

	return 0;

err_2:
	free(this);
err_1:
	return 1;
}

static void destroy(void** _this) {
	// TODO: destroy all nodes, walking up from leaves
	*_this = NULL;
	log_error("destroy not fully implemented");
}


const hash_api hash_bplustree = {
	.init = init,
	.destroy = destroy,

	.find = hashbplustree_find,
	.insert = hashbplustree_insert,
	.delete = hashbplustree_delete,

	.dump = hashbplustree_dump,

	.name = "hash_bplustree"
};
