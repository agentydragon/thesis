#include "dump.h"
#include "data.h"
#include "../../log/log.h"

#include <inttypes.h>

static void dump_walk(struct hashbplustree_node* node) {
	hashbplustree_dump_node(node);
	if (!node->is_leaf) {
		for (int8_t i = 0; i < node->keys_count + 1; i++) {
			hashbplustree_dump_node(node->pointers[i]);
		}
	}
}

void hashbplustree_dump(void* _this) {
	struct hashbplustree* this = _this;
	dump_walk(this->root);
}


void hashbplustree_dump_node(struct hashbplustree_node* target) {
	if (target->is_leaf) {
		printf("%p leaf ", target);
		for (int i = 0; i < target->keys_count; i++) {
			printf("%" PRIu64 "=%" PRIu64 " ",
					target->keys[i], target->values[i]);
		}
		printf("\n");
	} else {
		printf("%p inrl ", target);
		printf("%p ", target->pointers[0]);
		for (int i = 0; i < target->keys_count; i++) {
			printf(" <%" PRIu64 "> %p", target->keys[i],
					target->pointers[i + 1]);
		}
		printf("\n");
	}
}
