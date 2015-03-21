#include "ksplay/ksplay.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define NO_LOG_INFO
#include "log/log.h"
#include "util/unused.h"

#define node ksplay_node

// Port-in-progress from experiments/ksplay/ksplay.py.

static void _dump_single_node(node* x, int depth) {
	char buffer[256] = {0};
	int r = 0;
	for (int i = 0; i < depth; ++i) {
		r += sprintf(buffer + r, " ");
	}
	if ((long) x < 10000) {
		r += sprintf(buffer + r, "%p = MOCK(%ld)", x, (long) x);
	} else {
		r += sprintf(buffer + r, "%p (%" PRIu8 "): ", x, x->key_count);
		for (int i = 0; i < x->key_count; ++i) {
			r += sprintf(buffer + r, "%p <%" PRIu64 "=%" PRIu64 "> ",
					x->children[i],
					x->pairs[i].key, x->pairs[i].value);
		}
		r += sprintf(buffer + r, "%p", x->children[x->key_count]);
	}
	log_info("%s", buffer);
}

static void _dump_recursive(node* x, int depth) {
	_dump_single_node(x, depth);
	for (uint64_t i = 0; i <= x->key_count; ++i) {
		if (x->children[i]) {
			_dump_recursive(x->children[i], depth + 1);
		}
	}
}

void ksplay_dump(ksplay* tree) {
	_dump_recursive(tree->root, 0);
}

// Node.__init__
static node* node_init() {
	node* this = malloc(sizeof(node));
	assert(this);
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
	memmove(&this->children[before + 1], &this->children[before],
			sizeof(node*) * (this->key_count - before + 1));
	// TODO: move over all children
	this->pairs[before] = (ksplay_pair) {
		.key = key,
		.value = value
	};
	this->children[before] = NULL;
	++this->key_count;
	//this->children[this->key_count] = NULL;
	return true;
}

// Node.remove
static void node_remove_simple(node* this, uint64_t key) {
	uint8_t before;
	for (before = 0; before < this->key_count; ++before) {
		if (this->pairs[before].key >= key) {
			break;
		}
	}
	assert(before < this->key_count);
	assert(this->children[before + 1] == NULL);
	memmove(&this->pairs[before], &this->pairs[before + 1],
			sizeof(ksplay_pair) * (this->key_count - before - 1));
	memmove(&this->children[before + 1], &this->children[before + 2],
			sizeof(node*) * (this->key_count - before - 1));
	--this->key_count;
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
	this->size = 0;
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
	ksplay_pair* pairs = calloc(total_keys, sizeof(ksplay_pair));
	node** children = calloc(total_keys + 1, sizeof(node*));

	ksplay_pair* pairs_head = pairs;
	node** children_head = children;
	flatten_explore(stack, stack->nodes[0], &pairs_head, &children_head);

	*_pairs = pairs;
	*_children = children;
	*_key_count = total_keys;
}

static bool buffer_contains(ksplay_node_buffer* buffer, node* x) {
	for (uint8_t i = 0; i < buffer->count; ++i) {
		if (buffer->nodes[i] == x) {
			return true;
		}
	}
	return false;
}

static void flatten_explore(ksplay_node_buffer* stack, node* current,
		ksplay_pair** pairs_head, node*** children_head) {
	if (buffer_contains(stack, current)) {
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

static node* make_node(ksplay_pair* pairs, node** children, uint8_t key_count) {
	node* x = node_init();
	x->key_count = key_count;
	memcpy(x->pairs, pairs, key_count * sizeof(ksplay_pair));
	memcpy(x->children, children, (key_count + 1) * sizeof(node*));
	return x;
}

// Tree.compose
// Puts pairs and values under a new framework and returns its root.
// The new framework consists of at most three levels of nodes. All nodes
// except the root have exactly K-1 keys. The root will have between 1 and K
// keys.

node* compose_twolevel(ksplay_pair* pairs, node** children, uint64_t key_count) {
	// Split from left to right into entire nodes, then put
	// the rest into the root. The root will have at most KSPLAY_K
	// keys in the end.
	node* root = node_init();
	memset(root, 0, sizeof(node));
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

	assert(root->key_count <= KSPLAY_K && root->key_count > 0);

	log_info("Two-level composition:");
	_dump_single_node(root, 2);
	for (uint8_t i = 0; i < root->key_count + 1; ++i) {
		if (root->children[i]) {
			_dump_single_node(root->children[i], 4);
		}
	}

	return root;
}

static bool accepted_by_twolevel(uint64_t key_count) {
	const uint64_t child_count = key_count + 1;
	const uint64_t keys_needed_in_root =
			(child_count / KSPLAY_K) - 1 + // keys between full nodes
			(child_count % KSPLAY_K); // keys stashed in root
	return keys_needed_in_root <= KSPLAY_K;
}

// Let N be the number of composed keys.
// Case 1: N <= K --> it fits exactly into the root.
// Case 2: (N+1)/K + (N%K) <= K --> two-level hierarchy.
//	While you can, create full nodes and put them under the root.
//	Stash any remaining keys under the root.
// TODO: It should be possible to write an in-place 'compose'.
// TODO: Top-down K-splay to avoid allocating an O(N/K) stack?
node* ksplay_compose(ksplay_pair* pairs, node** children, uint64_t key_count) {
	log_info("Composing:");
	{
		char buffer[1024] = {0};
		uint64_t r = 0;
		for (uint64_t i = 0; i < key_count; ++i) {
			r += sprintf(buffer + r,
					"%p <%" PRIu64 "=%" PRIu64 "> ",
					children[i],
					pairs[i].key, pairs[i].value);
		}
		r += sprintf(buffer + r, "%p", children[key_count]);
		CHECK(r < sizeof(buffer), "we just smashed the stack");
		log_info("  %s", buffer);
	}
	if (key_count <= KSPLAY_K) {
		// Simple case: all fits in one node.
		log_info("One-level compose");
		return make_node(pairs, children, key_count);
	}
	if (accepted_by_twolevel(key_count)) {
		// Two-level case
		log_info("Two-level compose");
		return compose_twolevel(pairs, children, key_count);
	} else {
		log_info("Three-level compose");
		uint64_t lower_level = key_count;
		for (lower_level = key_count; lower_level > 0; --lower_level) {
			const uint64_t child_count = lower_level + 1;
			const uint64_t keys_needed_in_top =
					(child_count / KSPLAY_K) - 1 + // keys between full nodes
					(child_count % KSPLAY_K); // keys stashed in root
			if (keys_needed_in_top < KSPLAY_K) {
				break;
			}
		}
		assert(lower_level > 0);
		log_info("Three-level composition: %" PRIu64 " in full subtree",
				lower_level);
		assert(key_count - lower_level <= KSPLAY_K);
		node* middle = compose_twolevel(pairs, children, lower_level);

		node* root = node_init();
		root->key_count = key_count - lower_level;
		assert(root->key_count > 0);
		root->children[0] = middle;
		for (uint64_t i = lower_level; i < key_count; ++i) {
			root->pairs[i - lower_level] = pairs[i];
			root->children[i - lower_level + 1] = children[i + 1];
		}
		return root;
	}
}

static ksplay_node_buffer buffer_suffix(ksplay_node_buffer buffer,
		uint64_t suffix_length) {
	assert(buffer.count >= suffix_length);
	const uint64_t skip = buffer.count - suffix_length;
	return (ksplay_node_buffer) {
		.nodes = buffer.nodes + skip,
		.count = suffix_length,
		.capacity = suffix_length
	};
}

static void replace_pointer(node* haystack, node* needle, node* replacement) {
	bool done = false;
	for (uint64_t i = 0; i <= haystack->key_count; ++i) {
		if (haystack->children[i] == needle) {
			assert(!done);
			haystack->children[i] = replacement;
			done = true;
		}
	}
	assert(done);
}

// Tree.ksplay_step
// Does a single K-splaying step on the stack.
static void ksplay_step(ksplay_node_buffer* stack) {
	assert(stack->count > 1);

	log_info("K-splaying the following nodes:");
	for (uint64_t i = 0; i < stack->count; ++i) {
		_dump_single_node(stack->nodes[i], 2);
	}
	uint8_t suffix_length = KSPLAY_K;
	if (stack->nodes[stack->count - 1]->key_count + 1 >= KSPLAY_K) {
		++suffix_length;
	}
	if (stack->nodes[stack->count - 1]->key_count + 1 >= KSPLAY_K + 1) {
		++suffix_length;
	}

	const uint8_t consumed = (suffix_length < stack->count) ?
			suffix_length : stack->count;
	ksplay_node_buffer consumed_suffix = buffer_suffix(*stack, consumed);
	node* old_root = consumed_suffix.nodes[0];

	ksplay_pair* pairs;
	node** children;
	uint64_t key_count;
	ksplay_flatten(&consumed_suffix, &pairs, &children, &key_count);

	for (uint64_t i = 0; i < consumed; ++i) {
		free(consumed_suffix.nodes[i]);
	}

	log_info("stack before composition: %p", stack);
	stack->count -= consumed - 1;
	stack->nodes[stack->count - 1] =
			ksplay_compose(pairs, children, key_count);
	log_info("stack after composition: %p", stack);

	free(pairs);
	free(children);

	// Fix up possible pointers to old root.
	if (stack->count > 1) {
		// log_info("fixing pointer");
		node* new_root = stack->nodes[stack->count - 1];
		node* above_root = stack->nodes[stack->count - 2];
		replace_pointer(above_root, old_root, new_root);
	}

	log_info("K-splaying result:");
	for (uint64_t i = 0; i < stack->count; ++i) {
		_dump_single_node(stack->nodes[i], 2);
	}

	// Only the last node and the root may be non-exact.
	for (uint64_t i = 1; i < stack->count - 1; ++i) {
		assert(stack->nodes[i]->key_count == KSPLAY_K - 1);
	}

	// log_info("K-splay result:");
	// for (uint64_t i = 0; i < stack->count; ++i) {
	// 	_dump_single_node(stack->nodes[i], 2);
	// }
}

// Splits a node under a new one-key node if it's overfull.
node* ksplay_split_overfull(ksplay_node* root) {
	assert(root->key_count <= KSPLAY_K);
	if (root->key_count == KSPLAY_K) {
		log_info("Overfull root, splitting it:");
		//log_info("overfull root:::");
		_dump_single_node(root, 2);
		// Steal the last key.
		--root->key_count;

		node* new_root = node_init();
		new_root->key_count = 1;
		memcpy(&new_root->pairs[0], &root->pairs[root->key_count],
				sizeof(ksplay_pair));
		new_root->children[0] = root;
		new_root->children[1] = root->children[root->key_count + 1];

		log_info("Overfull root split into:");
		_dump_single_node(new_root, 2);
		_dump_single_node(root, 4);
		root = new_root;
	}
	return root;
}

// Tree.ksplay
// K-splays together the stack and returns the new root.
static node* _ksplay_ksplay(ksplay_node_buffer* stack) {
	log_info("-- starting new K-splay --");
	log_info("K-splaying input:");
	for (uint64_t i = 0; i < stack->count; ++i) {
		_dump_single_node(stack->nodes[i], 2);
	}
	while (stack->count > 1) {
		log_info("step");
		ksplay_step(stack);
	}
	// If the root is overfull, split it.
	node* root = ksplay_split_overfull(stack->nodes[0]);
	assert(root->key_count < KSPLAY_K);
	log_info("// K-splay done");

	return root;
}

static void ksplay_ksplay(ksplay* this, ksplay_node_buffer* stack) {
	this->root = _ksplay_ksplay(stack);
	if (this->size > 0) {
		assert(this->root->key_count > 0);
	}
}

// Tree.insert
int8_t ksplay_insert(ksplay* this, uint64_t key, uint64_t value) {
	log_info("before insert(%" PRIu64 "):", key);
	ksplay_dump(this);

	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target_node = stack.nodes[stack.count - 1];
	// TODO: tohle samo o sobe fungovat nebude...
	const int8_t result = node_insert(target_node, key, value) ? 0 : 1;
	if (result == 0) {
		++this->size;
	}
	ksplay_ksplay(this, &stack);

	log_info("after insert(%" PRIu64 "):", key);
	ksplay_dump(this);

	return result;
}

static int8_t node_contains(node* x, uint64_t key) {
	for (uint8_t i = 0; i < x->key_count; ++i) {
		if (x->pairs[i].key == key) {
			return true;
		}
	}
	return false;
}

static int8_t find_key_in_node(node* x, uint64_t key) {
	uint8_t i;
	for (i = 0; i < x->key_count; ++i) {
		if (x->pairs[i].key == key) {
			return i;
		}
	}
	log_fatal("key %" PRIu64 " not in node", key);
}

// Tree.delete
int8_t ksplay_delete(ksplay* this, uint64_t key) {
	log_info("before delete(%" PRIu64 "):", key);
	ksplay_dump(this);

	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target_node = stack.nodes[stack.count - 1];

	if (!node_contains(target_node, key)) {
		// No such key here.
		ksplay_ksplay(this, &stack);
		log_info("after failed delete(%" PRIu64 "):", key);
		ksplay_dump(this);
		return 1;
	}

	uint8_t index_in_target = find_key_in_node(target_node, key);
	if (target_node->children[index_in_target + 1] == NULL) {
		// Cool.
		log_info("delete: easy case");
		node_remove_simple(target_node, key);
		if (target_node->key_count == 0 && this->size > 1) {
			node* target_replacement = target_node->children[0];
			if (stack.count > 1) {
				replace_pointer(stack.nodes[stack.count - 2],
						target_node, target_replacement);
			}
			stack.nodes[stack.count - 1] = target_replacement;
			free(target_node);
		}
		--this->size;
		ksplay_ksplay(this, &stack);
		goto removed;
	} else {
		// Dude. Not cool.
		// Walk to least key.
		node* victim = target_node->children[index_in_target + 1];
		while (victim->children[0]) {
			buffer_append(&stack, victim);
			victim = victim->children[0];
		}
		assert(victim->key_count > 0);
		assert(victim->children[1] == NULL);
		ksplay_pair replacement = victim->pairs[0];
		node_remove_simple(victim, replacement.key);
		target_node->pairs[index_in_target + 1] = replacement;
		--this->size;
		ksplay_ksplay(this, &stack);
		goto removed;
	}

removed:
	log_info("after delete(%" PRIu64 "):", key);
	ksplay_dump(this);

	return 0;
}

// Tree.find
void ksplay_find(ksplay* this, uint64_t key, uint64_t *value, bool *found) {
	log_info("find(%" PRIu64 ")", key);
	// ksplay_dump(this);

	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target_node = stack.nodes[stack.count - 1];
	*found = node_find(target_node, key, value);
	ksplay_ksplay(this, &stack);

	// log_info("after find(%" PRIu64 "):", key);
	// ksplay_dump(this);
	// log_info("");
}
