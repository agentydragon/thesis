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

// TODO: It would be nice to have top-down K-splaying to avoid allocating
// huge stacks.

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

static void _dump_node_and_children(node* x, int depth) {
	_dump_single_node(x, depth);
	for (uint8_t i = 0; i <= node_key_count(x); ++i) {
		if (x->children[i]) {
			_dump_single_node(x->children[i], depth + 1);
		}
	}
}

static void _dump_buffer(ksplay_node_buffer* buffer, int depth) {
	for (uint64_t i = 0; i < buffer->count; ++i) {
		_dump_single_node(buffer->nodes[i], depth);
	}
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

static void node_init(node* this) {
	assert(this);
	for (uint8_t i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		this->pairs[i].key = EMPTY;
	}
	this->children[0] = NULL;
}

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
	this->pairs[before] = (ksplay_pair) {
		.key = key,
		.value = value
	};
	this->children[before] = NULL;
	return true;
}

static void node_remove_at_simple(node* this, uint8_t i) {
	const uint8_t key_count = node_key_count(this);
	CHECK(i < key_count, "removing beyond end");
	CHECK(this->children[i + 1] == NULL, "simply removing nonsimple case");
	memmove(&this->pairs[i], &this->pairs[i + 1],
			sizeof(ksplay_pair) * (key_count - i - 1));
	memmove(&this->children[i + 1], &this->children[i + 2],
			sizeof(node*) * (key_count - i - 1));
	this->pairs[key_count - 1].key = EMPTY;
}

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

void ksplay_init(ksplay* this) {
	this->root = malloc(sizeof(node));
	node_init(this->root);
	this->size = 0;
}

static void destroy_recursive(node** x) {
	node* p = *x;
	*x = NULL;
	// TODO: wat? copak to nekdy je cyklicke?
	const uint8_t key_count = node_key_count(p);
	for (uint8_t i = 0; i <= key_count; ++i) {
		if (p->children[i] != NULL) {
			destroy_recursive(&p->children[i]);
		}
	}
	//log_info("destroy %p", *x);
	free(p);
}

void ksplay_destroy(ksplay* this) {
	log_info("ksplay_destroy(%p)", this);
	destroy_recursive(&this->root);
	log_info("ksplay_destroy done");
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

static bool node_has_key_gt(node* x, uint64_t key) {
	for (uint64_t i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		if (x->pairs[i].key == EMPTY) {
			break;
		}
		if (x->pairs[i].key > key) {
			return true;
		}
	}
	return false;
}

static ksplay_node_buffer ksplay_walk_to_next(ksplay* this, uint64_t key,
		uint64_t *good_prefix) {
	ksplay_node_buffer stack = empty_buffer();
	node* current = this->root;

next_level:
	if (current == NULL) {
		goto drilldown_finished;
	}
	buffer_append(&stack, current);
	uint8_t i;
	for (i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
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

drilldown_finished:
	for (i = 0; i < stack.count; ++i) {
		if (node_has_key_gt(stack.nodes[stack.count - 1 - i], key)) {
			break;
		}
	}
	*good_prefix = stack.count - i;
	return stack;
}

static bool node_has_key_lt(node* x, uint64_t key) {
	for (uint64_t i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		if (x->pairs[i].key < key) {
			return true;
		}
	}
	return false;
}

static ksplay_node_buffer ksplay_walk_to_previous(ksplay* this, uint64_t key,
		uint64_t *good_prefix) {
	ksplay_node_buffer stack = empty_buffer();
	node* current = this->root;

next_level:
	if (current == NULL) {
		goto drilldown_finished;
	}
	buffer_append(&stack, current);
	uint8_t i;
	for (i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		const uint64_t idx = KSPLAY_MAX_NODE_KEYS - i - 1;
		if (current->pairs[idx].key < key) {
			current = current->children[idx + 1];
			goto next_level;
		}
	}
	current = current->children[0];
	goto next_level;

drilldown_finished:
	IF_LOG_VERBOSE(1) {
		log_info("stack in walk_to_previous:");
		_dump_buffer(&stack, 2);
	}
	for (i = 0; i < stack.count; ++i) {
		if (node_has_key_lt(stack.nodes[stack.count - 1 - i], key)) {
			break;
		}
	}
	log_verbose(1, "good prefix: %" PRIu64, stack.count - i);
	*good_prefix = stack.count - i;
	return stack;
}

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

static node* make_node(node* x, ksplay_pair* pairs, node** children,
		uint8_t key_count) {
	for (uint8_t i = key_count; i < KSPLAY_MAX_NODE_KEYS; ++i) {
		x->pairs[i].key = EMPTY;
	}
	memcpy(x->pairs, pairs, key_count * sizeof(ksplay_pair));
	memcpy(x->children, children, (key_count + 1) * sizeof(node*));
	return x;
}

// ----- Composition -----
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
		log_verbose(1, "acquired, %" PRIu64 " remaining", pool->remaining);
	} else {
		x = malloc(sizeof(node));
		// TODO: should this ever happen?
		// log_info("allocated new node (no more left in pool)");
	}
	assert(x);
	node_init(x);
	return x;
}

static node* compose_twolevel(ksplay_node_pool* pool,
		ksplay_pair* pairs, node** children, uint64_t key_count) {
	// Split from left to right into entire nodes, then put
	// the rest into the root. The root will have at most KSPLAY_K
	// keys in the end.
	node* root = pool_acquire(pool);
	uint8_t root_key_count = 0;

	uint64_t children_remaining = key_count + 1;
	while (children_remaining >= KSPLAY_K) {
		uint8_t keys_to_lower;
		if (children_remaining > KSPLAY_K) {
			keys_to_lower = KSPLAY_K - 1;
			// Not the last child; root eats the last key.
			root->pairs[root_key_count] = pairs[KSPLAY_K - 1];
		} else {
			// Last child.
			keys_to_lower = children_remaining - 1;
		}
		node* lower_node = make_node(pool_acquire(pool),
				pairs, children, keys_to_lower);
		root->children[root_key_count] = lower_node;

		pairs += KSPLAY_K;
		children += KSPLAY_K;
		if (children_remaining > KSPLAY_K) {
			++root_key_count;
		}
		children_remaining -= (keys_to_lower + 1);
	}

	if (children_remaining > 0) {
		memcpy(root->pairs + root_key_count, pairs,
				(children_remaining - 1) * sizeof(ksplay_pair));
		memcpy(root->children + root_key_count, children,
				children_remaining * sizeof(node*));
		root_key_count += children_remaining - 1;
	}

	assert(root_key_count == node_key_count(root));
	assert(root_key_count <= KSPLAY_K && root_key_count > 0);

	IF_LOG_VERBOSE(1) {
		log_info("Two-level composition:");
		_dump_node_and_children(root, 2);
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

static node* compose_threelevel(ksplay_node_pool *pool,
		ksplay_pair* pairs, node** children, uint64_t key_count) {
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
	node* middle = compose_twolevel(pool, pairs, children, lower_level);

	const uint8_t root_key_count = key_count - lower_level;
	assert(root_key_count > 0);
	node* root = pool_acquire(pool);
	root->children[0] = middle;
	for (uint64_t i = lower_level; i < key_count; ++i) {
		root->pairs[i - lower_level] = pairs[i];
		root->children[i - lower_level + 1] = children[i + 1];
	}
	assert(root_key_count == node_key_count(root));
	return root;
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
		return make_node(pool_acquire(pool),
				pairs, children, key_count);
	} else if (accepted_by_twolevel(key_count)) {
		log_info("Two-level compose");
		return compose_twolevel(pool, pairs, children, key_count);
	} else {
		log_info("Three-level compose");
		return compose_threelevel(pool, pairs, children, key_count);
	}
}

// ----- K-splaying -----
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

// Does a single K-splaying step on the stack.
static void ksplay_step(ksplay_node_buffer* stack) {
	assert(stack->count > 1);

	IF_LOG_VERBOSE(1) {
		log_info("K-splaying the following nodes:");
		_dump_buffer(stack, 2);
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

	// Fix up possible pointers to old root.
	if (stack->count > 2) {
		node* new_root = stack->nodes[stack->count - 1];
		node* above_root = stack->nodes[stack->count - 2];
		replace_pointer(above_root, old_root, new_root);
	}

	IF_LOG_VERBOSE(1) {
		log_info("K-splaying result:");
		_dump_buffer(stack, 2);
	}

	// Only the last node and the root may be non-exact.
	for (uint64_t i = 1; i < stack->count - 1; ++i) {
		assert(node_key_count(stack->nodes[i]) == KSPLAY_K - 1);
	}

	++KSPLAY_COUNTERS.ksplay_steps;
}

// Splits a node under a new one-key node if it's overfull.
node* ksplay_split_overfull(ksplay_node* root) {
	const uint8_t key_count = node_key_count(root);
	assert(key_count <= KSPLAY_K);
	if (key_count == KSPLAY_K) {
		IF_LOG_VERBOSE(1) {
			log_info("Overfull root, splitting it:");
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
			_dump_node_and_children(new_root, 2);
		}
		root = new_root;
	}
	return root;
}

// K-splays together the stack and returns the new root.
static void ksplay_ksplay(ksplay* this, ksplay_node_buffer* stack) {
	IF_LOG_VERBOSE(1) {
		log_info("-- starting new K-splay --");
		log_info("K-splaying input:");
		_dump_buffer(stack, 2);
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

// ----- Insertion and deletion -----
int8_t ksplay_insert(ksplay* this, uint64_t key, uint64_t value) {
	IF_LOG_VERBOSE(1) {
		log_info("before insert(%" PRIu64 "):", key);
		ksplay_dump(this);
	}

	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target = stack.nodes[stack.count - 1];
	const int8_t result = node_insert(target, key, value) ? 0 : 1;
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

int8_t ksplay_delete(ksplay* this, uint64_t key) {
	IF_LOG_VERBOSE(1) {
		log_info("before delete(%" PRIu64 "):", key);
		ksplay_dump(this);
	}

	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target = stack.nodes[stack.count - 1];
	node* victim;

	if (!node_contains(target, key)) {
		goto no_such_key;
	}

	uint8_t index_in_target = find_key_in_node(target, key);
	if (target->children[index_in_target + 1] == NULL) {
		// Easy case - we can simply remove the key and be done.
		node_remove_at_simple(target, index_in_target);
		victim = target;
		goto removed;
	} else {
		// Hard case - replace key with minimum of affected subtree,
		// then delete the old minimum.
		victim = target->children[index_in_target + 1];
		while (victim->children[0]) {
			buffer_append(&stack, victim);
			victim = victim->children[0];
		}
		target->pairs[index_in_target + 1] = victim->pairs[0];
		node_remove_at_simple(victim, 0);
		goto removed;
	}

no_such_key:
	ksplay_ksplay(this, &stack);
	IF_LOG_VERBOSE(1) {
		log_info("after failed delete(%" PRIu64 "):", key);
		ksplay_dump(this);
	}
	return 1;

removed:
	if (node_key_count(victim) == 0 && this->size > 1) {
		node* replacement = victim->children[0];
		if (stack.count > 1) {
			replace_pointer(stack.nodes[stack.count - 2],
					victim, replacement);
		}
		stack.nodes[stack.count - 1] = replacement;
		free(victim);
	}
	--this->size;
	ksplay_ksplay(this, &stack);
	IF_LOG_VERBOSE(1) {
		log_info("after delete(%" PRIu64 "):", key);
		ksplay_dump(this);
	}

	return 0;
}

void ksplay_find(ksplay* this, uint64_t key, uint64_t *value, bool *found) {
	IF_LOG_VERBOSE(1) {
		log_info("find(%" PRIu64 ")", key);
		ksplay_dump(this);
	}

	ksplay_node_buffer stack = ksplay_walk_to(this, key);
	node* target = stack.nodes[stack.count - 1];
	*found = node_find(target, key, value);
	ksplay_ksplay(this, &stack);

	IF_LOG_VERBOSE(1) {
		log_info("after find(%" PRIu64 "):", key);
		ksplay_dump(this);
	}
}

void ksplay_next_key(ksplay* this, uint64_t key,
		uint64_t *next_key, bool *found) {
	IF_LOG_VERBOSE(1) {
		log_info("next(%" PRIu64 ")", key);
		ksplay_dump(this);
	}

	uint64_t good_prefix;
	ksplay_node_buffer stack = ksplay_walk_to_next(this, key, &good_prefix);
	if (good_prefix == 0) {
		log_info("good prefix empty; next not found");
		*found = false;
	} else {
		node* good_node = stack.nodes[good_prefix - 1];
		uint64_t i;
		for (i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
			if (good_node->pairs[i].key == EMPTY) {
				continue;
			}
			if (good_node->pairs[i].key > key) {
				break;
			}
		}
		assert(i < KSPLAY_MAX_NODE_KEYS);
		*next_key = good_node->pairs[i].key;
		*found = true;
	}
	ksplay_ksplay(this, &stack);
}

void ksplay_previous_key(ksplay* this, uint64_t key,
		uint64_t *previous_key, bool *found) {
	IF_LOG_VERBOSE(1) {
		log_info("prev(%" PRIu64 ")", key);
		ksplay_dump(this);
	}

	uint64_t good_prefix;
	ksplay_node_buffer stack = ksplay_walk_to_previous(this, key,
			&good_prefix);
	if (good_prefix == 0) {
		*found = false;
	} else {
		node* good_node = stack.nodes[good_prefix - 1];
		uint64_t i;
		for (i = 0; i < KSPLAY_MAX_NODE_KEYS; ++i) {
			const uint64_t idx = KSPLAY_MAX_NODE_KEYS - i - 1;
			if (good_node->pairs[idx].key < key) {
				break;
			}
		}
		assert(i < KSPLAY_MAX_NODE_KEYS);
		*previous_key = good_node->pairs[
				KSPLAY_MAX_NODE_KEYS - i - 1].key;
		*found = true;
	}
	ksplay_ksplay(this, &stack);
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
