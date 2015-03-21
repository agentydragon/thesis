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

#define EMPTY UINT64_MAX

// Port-in-progress from experiments/ksplay/ksplay.py.

static uint8_t node_key_count(node* x) {
	uint8_t i;
	for (i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		if (x->pairs[i].key == EMPTY) {
			break;
		}
	}
	return i;
}

uint8_t ksplay_node_key_count(node* x) {
	return node_key_count(x);
}

static void _dump_single_node(node* x, int depth) {
	char buffer[256] = {0};
	int r = 0;
	for (int i = 0; i < depth; ++i) {
		r += sprintf(buffer + r, " ");
	}
	if ((long) x < 10000) {
		r += sprintf(buffer + r, "%p = MOCK(%ld)", x, (long) x);
	} else {
		const uint8_t key_count = node_key_count(x);
		r += sprintf(buffer + r, "%p (%" PRIu8 "): ", x, key_count);
		for (int i = 0; i < key_count; ++i) {
			r += sprintf(buffer + r, "%p <%" PRIu64 "=%" PRIu64 "> ",
					x->children[i],
					x->pairs[i].key, x->pairs[i].value);
		}
		r += sprintf(buffer + r, "%p", x->children[key_count]);
	}
	log_info("%s", buffer);
}

static void _dump_recursive(node* x, int depth) {
	_dump_single_node(x, depth);
	uint8_t key_count = node_key_count(x);
	for (uint64_t i = 0; i <= key_count; ++i) {
		if (x->children[i]) {
			_dump_recursive(x->children[i], depth + 1);
		}
	}
}

void ksplay_dump(ksplay* tree) {
	_dump_recursive(tree->root, 0);
}

// Node.__init__
static void node_init(node* this) {
	assert(this);
	for (uint8_t i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		this->pairs[i].key = EMPTY;
	}
	this->children[0] = NULL;
}

// Node.insert
static bool node_insert(node* this, uint64_t key, uint64_t value) {
	assert(key != EMPTY);
	uint8_t before;
	for (before = 0; before < KSPLAY_MAX_NODE_KEYS; ++before) {
		if (this->pairs[before].key == key) {
			// Duplicate key.
			return false;
		}
		if (this->pairs[before].key > key) {
			// Because empty keys are UINT64_MAX, this also happens
			// when we reach the end of a nonfull node.
			break;
		}
	}
	const uint8_t key_count = node_key_count(this);
	memmove(&this->pairs[before + 1], &this->pairs[before],
			sizeof(ksplay_pair) * (key_count - before));
	memmove(&this->children[before + 1], &this->children[before],
			sizeof(node*) * (key_count - before + 1));
	// TODO: move over all children
	this->pairs[before] = (ksplay_pair) {
		.key = key,
		.value = value
	};
	this->children[before] = NULL;
	return true;
}

// Node.remove
static void node_remove_simple(node* this, uint64_t key) {
	const uint8_t key_count = node_key_count(this);
	uint8_t before;
	for (before = 0; before < key_count; ++before) {
		if (this->pairs[before].key >= key) {
			break;
		}
	}
	assert(before < key_count);
	assert(this->children[before + 1] == NULL);
	memmove(&this->pairs[before], &this->pairs[before + 1],
			sizeof(ksplay_pair) * (key_count - before - 1));
	memmove(&this->children[before + 1], &this->children[before + 2],
			sizeof(node*) * (key_count - before - 1));
	this->pairs[key_count - 1].key = EMPTY;
	assert(node_key_count(this) == key_count - 1);
}

// Node.contains
static bool node_find(node* this, uint64_t key, uint64_t *value) {
	for (uint8_t i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
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
	this->root = malloc(sizeof(node));
	node_init(this->root);
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
	uint8_t i;
	for (i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		if (key == current->pairs[i].key) {
			goto finished;
		}
		if (current->pairs[i].key == EMPTY) {
			break;
		}
		if (key < current->pairs[i].key) {
			current = current->children[i];
			goto next_level;
		}
	}
	current = current->children[i];
	goto next_level;

finished:
	return stack;
}

// Tree.flatten
// Returns the pairs and external nodes of a stack in BFS order.
// 'pairs' should point to a buffer of at least KSPLAY_MAX_EXPLORE_KEYS
// ksplay_pairs. 'children' should point to at least KSPLAY_MAX_EXPLORE_KEYS + 1
// ksplay_node*'s.
static void flatten_explore(ksplay_node_buffer* stack, uint64_t current_index,
		ksplay_pair** pairs_head, node*** children_head,
		uint64_t* remaining_keys);
void ksplay_flatten(ksplay_node_buffer* stack, ksplay_pair* pairs,
		node** children, uint64_t* _key_count) {
	uint64_t total_keys = 0;
	for (uint64_t i = 0; i < stack->count; ++i) {
		total_keys += node_key_count(stack->nodes[i]);
	}
	assert(total_keys <= KSPLAY_MAX_EXPLORE_KEYS);

	ksplay_pair* pairs_head = pairs;
	node** children_head = children;
	uint64_t remaining = total_keys;
	flatten_explore(stack, 0, &pairs_head, &children_head, &remaining);
	assert(remaining == 0);

	*_key_count = total_keys;
}

static void flatten_explore(ksplay_node_buffer* stack, uint64_t current_index,
		ksplay_pair** pairs_head, node*** children_head,
		uint64_t* remaining_keys) {
	node* current = stack->nodes[current_index];
	const uint8_t key_count = node_key_count(current);
	for (uint8_t i = 0; i <= key_count; ++i) {
		if (stack->count > current_index + 1 &&
				stack->nodes[current_index + 1] == current->children[i]) {
			flatten_explore(stack, current_index + 1,
					pairs_head, children_head,
					remaining_keys);
		} else {
			*(*children_head) = current->children[i];
			++(*children_head);
		}

		if (i < key_count) {
			assert(*remaining_keys > 0);
			--(*remaining_keys);
			*(*pairs_head) = current->pairs[i];
			++(*pairs_head);
		}
	}
}

static void make_node(node* x, ksplay_pair* pairs, node** children,
		uint8_t key_count) {
	for (uint8_t i = key_count; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		x->pairs[i].key = EMPTY;
	}
	memcpy(x->pairs, pairs, key_count * sizeof(ksplay_pair));
	memcpy(x->children, children, (key_count + 1) * sizeof(node*));
}

// Tree.compose
// Puts pairs and values under a new framework and returns its root.
// The new framework consists of at most three levels of nodes. All nodes
// except the root have exactly K-1 keys. The root will have between 1 and K
// keys.

static node* pool_acquire(ksplay_node_pool* pool) {
	node* x;
	if (pool->remaining > 0) {
		x = pool->nodes[0];
		++(pool->nodes);
		--(pool->remaining);
		log_info("acquired, %" PRIu64 " remaining", pool->remaining);
	} else {
		x = malloc(sizeof(node));
		log_info("allocated new node (no more left in pool)");
	}
	assert(x);
	node_init(x);
	return x;
}

node* compose_twolevel(ksplay_node_pool* pool,
		ksplay_pair* pairs, node** children, uint64_t key_count) {
	// Split from left to right into entire nodes, then put
	// the rest into the root. The root will have at most KSPLAY_K
	// keys in the end.
	node* root = pool_acquire(pool);
	uint8_t root_key_count = 0;

	uint64_t children_remaining = key_count + 1;
	while (children_remaining >= KSPLAY_K) {
		node* lower_node = pool_acquire(pool);
		root->children[root_key_count] = lower_node;
		if (children_remaining > KSPLAY_K) {
			memcpy(lower_node->pairs, pairs,
					(KSPLAY_K - 1) * sizeof(ksplay_pair));
			memcpy(lower_node->children, children,
					KSPLAY_K * sizeof(node*));
			assert(node_key_count(lower_node) == KSPLAY_K - 1);
			root->pairs[root_key_count] = pairs[KSPLAY_K - 1];
			++root_key_count;
		} else {
			// Last child.
			memcpy(lower_node->pairs, pairs,
					(children_remaining - 1) * sizeof(ksplay_pair));
			memcpy(lower_node->children, children,
					children_remaining * sizeof(node*));
			assert(node_key_count(lower_node) ==
					children_remaining - 1);
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
		memcpy(root->pairs + root_key_count,
				pairs, (children_remaining - 1) * sizeof(ksplay_pair));
		memcpy(root->children + root_key_count,
				children, children_remaining * sizeof(node*));
		root_key_count += children_remaining - 1;
	}

	assert(root_key_count == node_key_count(root));
	assert(root_key_count <= KSPLAY_K && root_key_count > 0);

	IF_LOG_VERBOSE(1) {
		log_info("Two-level composition:");
		_dump_single_node(root, 2);
		for (uint8_t i = 0; i <= root_key_count; ++i) {
			if (root->children[i]) {
				_dump_single_node(root->children[i], 4);
			}
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
node* ksplay_compose(ksplay_node_pool* pool, ksplay_pair* pairs, node** children,
		uint64_t key_count) {
	IF_LOG_VERBOSE(1) {
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
	}
	if (key_count <= KSPLAY_K) {
		// Simple case: all fits in one node.
		log_info("One-level compose");
		node* root = pool_acquire(pool);
		make_node(root, pairs, children, key_count);
		return root;
	}
	if (accepted_by_twolevel(key_count)) {
		// Two-level case
		log_info("Two-level compose");
		return compose_twolevel(pool, pairs, children, key_count);
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
		node* middle = compose_twolevel(pool,
				pairs, children, lower_level);

		node* root = pool_acquire(pool);
		const uint8_t root_key_count = key_count - lower_level;
		assert(root_key_count > 0);
		root->children[0] = middle;
		for (uint64_t i = lower_level; i < key_count; ++i) {
			root->pairs[i - lower_level] = pairs[i];
			root->children[i - lower_level + 1] = children[i + 1];
		}
		assert(root_key_count == node_key_count(root));
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
	for (uint64_t i = 0; i <= node_key_count(haystack); ++i) {
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

	IF_LOG_VERBOSE(1) {
		log_info("K-splaying the following nodes:");
		for (uint64_t i = 0; i < stack->count; ++i) {
			_dump_single_node(stack->nodes[i], 2);
		}
	}
	uint8_t suffix_length = KSPLAY_K;
	const uint8_t keys_in_last =
			node_key_count(stack->nodes[stack->count - 1]);
	if (keys_in_last + 1 >= KSPLAY_K) {
		++suffix_length;
	}
	if (keys_in_last + 1 >= KSPLAY_K + 1) {
		++suffix_length;
	}

	const uint8_t consumed = (suffix_length < stack->count) ?
			suffix_length : stack->count;
	ksplay_node_buffer consumed_suffix = buffer_suffix(*stack, consumed);
	node* old_root = consumed_suffix.nodes[0];

	ksplay_pair pairs[KSPLAY_MAX_EXPLORE_KEYS];
	node* children[KSPLAY_MAX_EXPLORE_KEYS + 1];;
	uint64_t key_count;
	ksplay_flatten(&consumed_suffix, pairs, children, &key_count);
	assert(key_count <= (KSPLAY_K + 2) * (KSPLAY_K - 1) + 1);

	ksplay_node* nodes[100]; // TODO
	assert(consumed < 100);
	ksplay_node_pool pool;
	pool.nodes = nodes;
	pool.remaining = consumed;
	for (uint64_t i = 0; i < consumed; ++i) {
		pool.nodes[i] = consumed_suffix.nodes[i];
	}

	log_info("stack before composition: %p", stack);
	stack->count -= consumed - 1;
	stack->nodes[stack->count - 1] =
			ksplay_compose(&pool, pairs, children, key_count);
	log_info("stack after composition: %p", stack);

	while (pool.remaining > 0) {
		free(pool_acquire(&pool));
	}

	//free(pairs);
	//free(children);

	// Fix up possible pointers to old root.
	if (stack->count > 2) {
		// log_info("fixing pointer");
		node* new_root = stack->nodes[stack->count - 1];
		node* above_root = stack->nodes[stack->count - 2];
		replace_pointer(above_root, old_root, new_root);
	}

	IF_LOG_VERBOSE(1) {
		log_info("K-splaying result:");
		for (uint64_t i = 0; i < stack->count; ++i) {
			_dump_single_node(stack->nodes[i], 2);
		}
	}

	// Only the last node and the root may be non-exact.
	for (uint64_t i = 1; i < stack->count - 1; ++i) {
		assert(node_key_count(stack->nodes[i]) == KSPLAY_K - 1);
	}
}

// Splits a node under a new one-key node if it's overfull.
node* ksplay_split_overfull(ksplay_node* root) {
	const uint8_t key_count = node_key_count(root);
	assert(key_count <= KSPLAY_K);
	if (key_count == KSPLAY_K) {
		IF_LOG_VERBOSE(1) {
			log_info("Overfull root, splitting it:");
			//log_info("overfull root:::");
			_dump_single_node(root, 2);
		}
		// Steal the last key.

		node* new_root = malloc(sizeof(node));
		node_init(new_root);
		memcpy(&new_root->pairs[0], &root->pairs[key_count - 1],
				sizeof(ksplay_pair));
		new_root->children[0] = root;
		new_root->children[1] = root->children[key_count];
		assert(node_key_count(new_root) == 1);

		root->pairs[key_count - 1].key = EMPTY;
		assert(node_key_count(root) == key_count - 1);

		IF_LOG_VERBOSE(1) {
			log_info("Overfull root split into:");
			_dump_single_node(new_root, 2);
			_dump_single_node(root, 4);
		}
		root = new_root;
	}
	return root;
}

// Tree.ksplay
// K-splays together the stack and returns the new root.
static void ksplay_ksplay(ksplay* this, ksplay_node_buffer* stack) {
	IF_LOG_VERBOSE(1) {
		log_info("-- starting new K-splay --");
		log_info("K-splaying input:");
		for (uint64_t i = 0; i < stack->count; ++i) {
			_dump_single_node(stack->nodes[i], 2);
		}
	}
	while (stack->count > 1) {
		log_info("step");
		ksplay_step(stack);
	}
	// If the root is overfull, split it.
	node* root = ksplay_split_overfull(stack->nodes[0]);
	assert(node_key_count(root) < KSPLAY_K);
	this->root = root;
	log_info("// K-splay done");
	if (this->size > 0) {
		assert(node_key_count(this->root) > 0);
	}
}

// Tree.insert
int8_t ksplay_insert(ksplay* this, uint64_t key, uint64_t value) {
	IF_LOG_VERBOSE(1) {
		log_info("before insert(%" PRIu64 "):", key);
		ksplay_dump(this);
	}

	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target_node = stack.nodes[stack.count - 1];
	const int8_t result = node_insert(target_node, key, value) ? 0 : 1;
	if (result == 0) {
		++this->size;
	}
	ksplay_ksplay(this, &stack);

	IF_LOG_VERBOSE(1) {
		log_info("after insert(%" PRIu64 "):", key);
		ksplay_dump(this);
	}

	return result;
}

static int8_t node_contains(node* x, uint64_t key) {
	for (uint8_t i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		if (x->pairs[i].key == key) {
			return true;
		}
		if (x->pairs[i].key == EMPTY) {
			break;
		}
	}
	return false;
}

static int8_t find_key_in_node(node* x, uint64_t key) {
	uint8_t i;
	for (i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		if (x->pairs[i].key == key) {
			return i;
		}
		if (x->pairs[i].key == EMPTY) {
			break;
		}
	}
	log_fatal("key %" PRIu64 " not in node", key);
}

// Tree.delete
int8_t ksplay_delete(ksplay* this, uint64_t key) {
	IF_LOG_VERBOSE(1) {
		log_info("before delete(%" PRIu64 "):", key);
		ksplay_dump(this);
	}

	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target_node = stack.nodes[stack.count - 1];

	if (!node_contains(target_node, key)) {
		// No such key here.
		ksplay_ksplay(this, &stack);
		IF_LOG_VERBOSE(1) {
			log_info("after failed delete(%" PRIu64 "):", key);
			ksplay_dump(this);
		}
		return 1;
	}

	uint8_t index_in_target = find_key_in_node(target_node, key);
	if (target_node->children[index_in_target + 1] == NULL) {
		// Cool.
		log_info("delete: easy case");
		node_remove_simple(target_node, key);
		if (node_key_count(target_node) == 0 && this->size > 1) {
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
		assert(node_key_count(victim) > 0);
		assert(victim->children[1] == NULL);
		ksplay_pair replacement = victim->pairs[0];
		node_remove_simple(victim, replacement.key);
		target_node->pairs[index_in_target + 1] = replacement;
		--this->size;
		ksplay_ksplay(this, &stack);
		goto removed;
	}

removed:
	IF_LOG_VERBOSE(1) {
		log_info("after delete(%" PRIu64 "):", key);
		ksplay_dump(this);
	}

	return 0;
}

// Tree.find
void ksplay_find(ksplay* this, uint64_t key, uint64_t *value, bool *found) {
	IF_LOG_VERBOSE(1) {
		log_info("find(%" PRIu64 ")", key);
		ksplay_dump(this);
	}

	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target_node = stack.nodes[stack.count - 1];
	*found = node_find(target_node, key, value);
	ksplay_ksplay(this, &stack);

	IF_LOG_VERBOSE(1) {
		log_info("after find(%" PRIu64 "):", key);
		ksplay_dump(this);
	}
}

static void _dump_dot(node* current_node, FILE* output) {
	fprintf(output, "    node%p[label = \"", current_node);
	const uint8_t key_count = node_key_count(current_node);
	for (uint8_t i = 0; i < key_count; ++i) {
		fprintf(output, "<child%d>|%" PRIu64 "|",
				i, current_node->pairs[i].key);
	}
	fprintf(output, "<child%d>\"];\n", key_count);

	for (uint8_t i = 0; i <= key_count; ++i) {
		if (current_node->children[i]) {
			fprintf(output, "    \"node%p\":child%d -> \"node%p\";\n",
					current_node, i, current_node->children[i]);
		}
	}

	for (uint8_t i = 0; i <= key_count; ++i) {
		if (current_node->children[i]) {
			_dump_dot(current_node->children[i], output);
		}
	}
}

void ksplay_dump_dot(ksplay* this, FILE* output) {
	fprintf(output, "digraph ksplay {\n");
	fprintf(output, "    node [shape = record, height = .1];\n");
	_dump_dot(this->root, output);
	fprintf(output, "}\n");
}
