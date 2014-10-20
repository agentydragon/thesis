#include "hash_bplustree.h"
#include "private/find.h"
#include "private/insert.h"
#include "private/delete.h"
#include "private/dump.h"
#include "private/data.h"
#include "private/helpers.h"

#include "../log/log.h"

#include <stdlib.h>

static int8_t init(void** _this, void* args_unused) {
	(void) args_unused;

	struct hashbplustree* this = malloc(sizeof(tree));
	if (!this) {
		goto err_1;
	}

	this->root = malloc(sizeof(node));
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

static void destroy_recursive(node* target) {
	if (!target->is_leaf) {
		for (int8_t i = 0; i < target->keys_count + 1; i++) {
			destroy_recursive(target->pointers[i]);
		}
	}
	free(target);
}

static void destroy(void** _this) {
	if (_this) {
		tree* this = *_this;
		if (this) {
			destroy_recursive(this->root);
			free(this);
		}
	}
	*_this = NULL;
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
