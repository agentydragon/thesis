#include "dump.h"
#include "data.h"
#include "../../log/log.h"

#include <inttypes.h>

void hashbplustree_dump(void* _this) {
	(void) _this;
	log_error("not implemented");
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
