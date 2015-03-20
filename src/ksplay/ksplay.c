#include "ksplay/ksplay.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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
static bool node_insert(node* this, uint64_t key, uint64_t value) {
	assert(this->key_count < KSPLAY_MAX_NODE_KEYS);
	uint8_t before;
	for (before = 0; before < this->key_count; ++before) {
		if (this->pairs[before].key == key) {
			// Duplicate key.
			return false;
		}
		if (this->pairs[before].key > key) {
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
	return true;
}

// Node.remove
static bool node_remove(node* this, uint64_t key) {
	assert(this->key_count > 0);
	uint8_t before;
	for (before = 0; before < this->key_count; ++before) {
		if (this->pairs[before].key >= key) {
			break;
		}
	}
	if (before == this->key_count) {
		// No such key here.
		return false;
	}
	memmove(&this->pairs[before], &this->pairs[before + 1],
			sizeof(ksplay_pair) * (this->key_count - before - 1));
	--this->key_count;
	return true;
}

// Node.contains
static bool node_find(node* this, uint64_t key, uint64_t *value) {
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

static ksplay_node_buffer empty_buffer() {
	return (ksplay_node_buffer) {
		.nodes = NULL,
		.count = 0,
		.capacity = 0
	};
}

static void buffer_append(ksplay_node_buffer* buffer, node* appended_node) {
	if (buffer->count == buffer->capacity) {
		if (buffer->capacity < 1) {
			buffer->capacity = 1;
		}
		buffer->capacity *= 2;
		buffer->nodes = realloc(buffer->nodes,
				sizeof(node*) * buffer->capacity);
	}
	buffer->nodes[buffer->count] = appended_node;
	++buffer->count;
}

// Tree.walk_to
ksplay_node_buffer ksplay_walk_to(ksplay* this, uint64_t key) {
	ksplay_node_buffer stack = empty_buffer();
	node* current = this->root;

next_level:
	if (current == NULL) {
		goto finished;
	}
	buffer_append(&stack, current);
	for (uint8_t i = 0; i < current->key_count; ++i) {
		if (key == current->pairs[i].key) {
			goto finished;
		}
		if (key < current->pairs[i].key) {
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
static void flatten_explore(ksplay_node_buffer* stack, node* current,
		ksplay_pair** pairs_head, node*** children_head);
void ksplay_flatten(ksplay_node_buffer* stack, ksplay_pair** _pairs,
		node*** _children, uint64_t* _key_count) {
	uint64_t total_keys = 0;
	for (uint64_t i = 0; i < stack->count; ++i) {
		total_keys += stack->nodes[i]->key_count;
	}
	log_info("total keys = %" PRIu64, total_keys);
	ksplay_pair* pairs = calloc(total_keys, sizeof(ksplay_pair));
	node** children = calloc(total_keys + 1, sizeof(node*));

	ksplay_pair* pairs_head = pairs;
	node** children_head = children;
	flatten_explore(stack, stack->nodes[0], &pairs_head, &children_head);

	*_pairs = pairs;
	*_children = children;
	*_key_count = total_keys;
}

static void flatten_explore(ksplay_node_buffer* stack, node* current,
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
				current->children[current->key_count],
				pairs_head, children_head);
	} else {
		*(*children_head) = current;
		++(*children_head);
	}
}

// Tree.compose
// Puts pairs and values under a new framework and returns its root.
// TODO: It should be possible to write an in-place 'compose'.
// TODO: Top-down K-splay to avoid allocating an O(N/K) stack?
node* ksplay_compose(ksplay_pair* pairs, node** children, uint64_t key_count) {
	node* root = node_init();
	memset(root, 0, sizeof(node));
	if (key_count <= KSPLAY_K) {
		root->key_count = key_count;
		memcpy(&root->pairs, pairs, key_count * sizeof(ksplay_pair));
		memcpy(&root->children, children,
				(key_count + 1) * sizeof(node*));
	} else {
		root->key_count = 0;

		uint64_t children_remaining = key_count + 1;
		while (children_remaining >= KSPLAY_K) {
			node* lower_node = node_init();
			root->children[root->key_count] = lower_node;
			if (children_remaining > KSPLAY_K) {
				lower_node->key_count = KSPLAY_K - 1;
				memcpy(lower_node->pairs, pairs,
						(KSPLAY_K - 1) * sizeof(ksplay_pair));
				memcpy(lower_node->children, children,
						KSPLAY_K * sizeof(node*));
				root->pairs[root->key_count] =
						pairs[KSPLAY_K - 1];
				++root->key_count;
			} else {
				// Last child.
				lower_node->key_count = children_remaining - 1;
				memcpy(lower_node->pairs, pairs,
						(children_remaining - 1) * sizeof(ksplay_pair));
				memcpy(lower_node->children, children,
						children_remaining * sizeof(node*));
			}
			pairs += KSPLAY_K;
			children += KSPLAY_K;
			if (children_remaining > KSPLAY_K) {
				children_remaining -= KSPLAY_K;
			} else {
				children_remaining = 0;
			}
		}

		//assert(children_remaining == 0);
		if (children_remaining > 0) {
			memcpy(root->pairs + root->key_count,
					pairs, (children_remaining - 1) * sizeof(ksplay_pair));
			memcpy(root->children + root->key_count,
					children, children_remaining * sizeof(node*));
			root->key_count += children_remaining - 1;
		}
	}
	return root;
}

// Tree.ksplay_step
// Does a single K-splaying step on the stack.
static UNUSED void ksplay_step(ksplay_node_buffer* stack) {
	(void) stack;
	log_fatal("TODO: port over ksplay_step");
}

// Tree.ksplay
// K-splays together the stack and returns the new root.
static UNUSED node* ksplay_ksplay(ksplay_node_buffer* stack) {
	(void) stack;
	log_fatal("TODO: port over ksplay");
	return NULL;
}

// Tree.insert
int8_t ksplay_insert(ksplay* this, uint64_t key, uint64_t value) {
	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target_node = stack.nodes[stack.count - 1];
	const int8_t result = node_insert(target_node, key, value) ? 0 : 1;
	this->root = ksplay_ksplay(&stack);
	return result;
}

// Tree.delete
int8_t ksplay_delete(ksplay* this, uint64_t key) {
	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target_node = stack.nodes[stack.count - 1];
	const int8_t result = node_remove(target_node, key) ? 0 : 1;
	this->root = ksplay_ksplay(&stack);
	return result;
}

// Tree.find
void ksplay_find(ksplay* this, uint64_t key, uint64_t *value, bool *found) {
	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target_node = stack.nodes[stack.count - 1];
	*found = node_find(target_node, key, value);
	this->root = ksplay_ksplay(&stack);
}
