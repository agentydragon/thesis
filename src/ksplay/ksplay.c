#include "ksplay/ksplay.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "log/log.h"
#include "util/unused.h"

#define node ksplay_node

// Port-in-progress from experiments/ksplay/ksplay.py.

// Node.__init__
static node* node_init() {
	node* this = malloc(sizeof(node));
	this->key_count = 0;
	this->children[0] = NULL;
	return this;
}

// Node.insert
static UNUSED void node_insert(node* this, uint64_t key, uint64_t value) {
	assert(this->key_count < KSPLAY_MAX_NODE_KEYS);
	uint8_t before;
	for (before = 0; before < this->key_count; ++before) {
		if (this->pairs[before].key >= key) {
			break;
		}
	}
	memmove(&this->pairs[before + 1], &this->pairs[before],
			sizeof(ksplay_pair) * (this->key_count - before));
	this->pairs[before] = (ksplay_pair) {
		.key = key,
		.value = value
	};
	++this->key_count;
}

// Node.remove
static UNUSED void node_remove(node* this, uint64_t key) {
	assert(this->key_count > 0);
	uint8_t before;
	for (before = 0; before < this->key_count; ++before) {
		if (this->pairs[before].key >= key) {
			break;
		}
	}
	CHECK(before < this->key_count, "deleting nonexistant key from node");
	memmove(&this->pairs[before], &this->pairs[before + 1],
			sizeof(ksplay_pair) * (this->key_count - before - 1));
	--this->key_count;
}

// Node.contains
static UNUSED bool node_find(node* this, uint64_t key, uint64_t *value) {
	for (uint8_t i = 0; i < this->key_count; ++i) {
		if (this->pairs[i].key == key) {
			if (value) {
				*value = this->pairs[i].value;
			}
			return true;
		}
	}
	return false;
}

// Tree.__init__
void ksplay_init(ksplay* this) {
	this->root = node_init();
}

void ksplay_destroy(ksplay* this) {
	(void) this;
	log_error("TODO: implement ksplay_destroy");
}

typedef struct {
	node** nodes;
	uint64_t count;
	uint64_t capacity;
} node_buffer;

static node_buffer empty_buffer() {
	return (node_buffer) {
		.nodes = NULL,
		.count = 0,
		.capacity = 0
	};
}

static void buffer_append(node_buffer* buffer, node* appended_node) {
	if (buffer->count == buffer->capacity) {
		if (buffer->capacity < 1) {
			buffer->capacity = 1;
		}
		buffer->capacity *= 2;
		buffer->nodes = realloc(buffer->nodes,
				sizeof(node*) * buffer->capacity);
	}
	buffer->nodes[++buffer->count] = appended_node;
}

// Tree.walk_to
static UNUSED node_buffer walk_to(ksplay* this, uint64_t key) {
	node_buffer stack = empty_buffer();
	node* current = this->root;

next_level:
	buffer_append(&stack, current);
	for (uint8_t i = 0; i < current->key_count; ++i) {
		if (current->pairs[i].key == key) {
			goto finished;
		}
		if (current->pairs[i].key < key) {
			current = current->children[i];
			goto next_level;
		}
	}
	if (current->key_count > 0) {
		current = current->children[current->key_count];
		goto next_level;
	} else {
		goto finished;
	}

finished:
	return stack;
}

// Tree.flatten
// Returns the pairs and external nodes of a stack in BFS order.
// TODO: 'pairs' and 'children' have a fixed maximum size of O(K^2),
// this should be doable without dynamic allocation.
static void flatten_explore(node_buffer* stack, node* current,
		ksplay_pair** pairs_head, node*** children_head);
static UNUSED void flatten(node_buffer* stack,
		ksplay_pair** _pairs, node*** _children) {
	uint64_t total_keys = 0;
	for (uint64_t i = 0; i < stack->count; ++i) {
		total_keys += stack->nodes[i]->key_count;
	}
	ksplay_pair* pairs = calloc(total_keys, sizeof(ksplay_pair));
	node** children = calloc(total_keys + 1, sizeof(node*));

	ksplay_pair* pairs_head = pairs;
	node** children_head = children;
	flatten_explore(stack, stack->nodes[0], &pairs_head, &children_head);

	*_pairs = pairs;
	*_children = children;
}

static void flatten_explore(node_buffer* stack, node* current,
		ksplay_pair** pairs_head, node*** children_head) {
	bool found = false;
	for (uint8_t i = 0; i < stack->count; ++i) {
		if (stack->nodes[i] == current) {
			found = true;
			break;
		}
	}
	if (found) {
		for (uint8_t i = 0; i < current->key_count; ++i) {
			flatten_explore(stack, current->children[i],
					pairs_head, children_head);
			*(*pairs_head) = current->pairs[i];
			++(*pairs_head);
		}
		flatten_explore(stack,
				current->children[current->key_count - 1],
				pairs_head, children_head);
	} else {
		*(*children_head) = current;
		++(*children_head);
	}
}

// Tree.compose
// Puts pairs and values under a new framework and returns its root.
static UNUSED node* compose(ksplay_pair* pairs, node* children,
		uint64_t key_count) {
	node* root = node_init();
	if (key_count <= KSPLAY_K) {
		root->key_count = key_count;
		memcpy(&root->pairs, pairs, key_count * sizeof(ksplay_pair));
		memcpy(&root->children, children,
				(key_count + 1) * sizeof(node*));
	} else {
		root->key_count = 0;
		log_fatal("TODO: port over pair-splitting logic");
	}
	return root;
}

// Tree.ksplay_step
// Does a single K-splaying step on the stack.
static UNUSED void ksplay_step(node_buffer* stack) {
	(void) stack;
	log_fatal("TODO: port over ksplay_step");
}

// Tree.ksplay
// K-splays together the stack and returns the new root.
static UNUSED void ksplay_ksplay(node_buffer* stack) {
	(void) stack;
	log_fatal("TODO: port over ksplay");
}

// Tree.insert
void ksplay_insert(ksplay* this, uint64_t key, uint64_t value) {
	(void) this; (void) key; (void) value;
	log_fatal("TODO: port over ksplay_insert");
}

// Tree.delete
void ksplay_delete(ksplay* this, uint64_t key) {
	(void) this; (void) key;
	log_fatal("TODO: port over ksplay_delete");
}

// Tree.find
void ksplay_find(ksplay* this, uint64_t key, uint64_t *value, bool *found) {
	(void) this; (void) key; (void) value; (void) found;
	log_fatal("TODO: port over ksplay_find");
}
