#include "dump.h"
#include "data.h"
#include "helpers.h"
#include "../../log/log.h"

#include <inttypes.h>

/*
static void dump_walk(node* target) {
	hashbplustree_dump_node(target);
	if (!target->is_leaf) {
		for (int8_t i = 0; i < target->keys_count + 1; i++) {
			hashbplustree_dump_node(target->pointers[i]);
		}
	}
}
*/

static void walk_dfs(node* target,
		void (*callback_enter)(node*, void*),
		void (*callback_leave)(node*, void*),
		void* opaque) {
	callback_enter(target, opaque);
	if (!target->is_leaf) {
		for (int8_t i = 0; i < target->keys_count + 1; i++) {
			walk_dfs(target->pointers[i],
					callback_enter, callback_leave, opaque);
		}
	}
	callback_leave(target, opaque);
}

struct depths {
	int depths[100];
	int depth;
};

static void calculate_depths_enter(node* target, void* opaque) {
	(void) target;
	struct depths* depths = opaque;
	depths->depths[depths->depth++]++;
}

static void calculate_depths_leave(node* target, void* opaque) {
	(void) target;
	struct depths* depths = opaque;
	depths->depth--;
}

void hashbplustree_dump(void* _this) {
	struct hashbplustree* this = _this;
	struct depths arg = {
		.depths = { 0 },
		.depth = 0
	};
	walk_dfs(this->root,
			calculate_depths_enter, calculate_depths_leave, &arg);

	for (int i = 0; i < 100; i++) {
		if (arg.depths[i] > 0) {
			printf("depth %d: %d\n", i, arg.depths[i]);
		}
	}
}


void hashbplustree_dump_node(struct hashbplustree_node* target) {
	if (target->is_leaf) {
		printf("%p L ", target);
		for (int i = 0; i < target->keys_count; i++) {
			printf("%" PRIu64 "=%" PRIu64 " ",
					target->keys[i], target->values[i]);
		}
		printf("\n");
	} else {
		printf("%p I %p", target, target->pointers[0]);
		for (int i = 0; i < target->keys_count; i++) {
			printf(" <%" PRIu64 "> %p", target->keys[i],
					target->pointers[i + 1]);
		}
		printf("\n");
	}
}
