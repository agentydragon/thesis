#include "btree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../log/log.h"

// TODO: static_assert's for min-keys/max-keys conditions

// Details of node representation:
static btree_node_persisted* new_empty_leaf();
static btree_node_persisted* new_fork_node(uint64_t middle_key,
		btree_node_persisted* left, btree_node_persisted* right);
void split_leaf(btree_node_persisted* node,
		btree_node_persisted* new_right_sibling, uint64_t *middle_key);
void split_internal(btree_node_persisted* node,
		btree_node_persisted* new_right_sibling, uint64_t *middle_key);
void insert_pointer(btree_node_persisted* node, uint64_t key,
		btree_node_persisted* pointer);
static void remove_ptr_from_node(btree_node_persisted* parent,
		const btree_node_persisted* remove);
int8_t insert_key_value_pair(btree_node_persisted* leaf,
		uint64_t key, uint64_t value);
static bool remove_from_leaf(btree_node_persisted* leaf, uint64_t key);
static void rebalance_internal(btree_node_persisted* parent,
		btree_node_persisted* left, btree_node_persisted* right,
		const uint8_t right_index,
		const uint8_t to_left, const uint8_t to_right);
static void rebalance_leaves(btree_node_persisted* parent,
		btree_node_persisted* left, btree_node_persisted* right,
		const uint8_t right_index,
		const uint8_t to_left, const uint8_t to_right);
static void append_internal(btree_node_persisted* target, uint64_t appended_key,
		const btree_node_persisted* source);
static void append_leaf(btree_node_persisted* target,
		const btree_node_persisted* source);
static uint8_t get_n_keys(const btree_node_persisted* node);
static void find_siblings(btree_node_persisted* node,
		btree_node_persisted* parent,
		btree_node_persisted** left, btree_node_persisted** right,
		uint8_t* right_index);
#define FOR_EACH_INTERNAL_POINTER(persisted_node,code) do { \
	for (uint8_t _i = 0; _i < (persisted_node).key_count + 1; ++_i) { \
		btree_node_persisted* const pointer = (persisted_node).pointers[_i]; \
		code \
	} \
} while (0)

// B-tree "meta-algorithm":
enum side_preference { LEFT, RIGHT };

typedef struct {
	uint8_t levels_above_leaves;
	btree_node_persisted* persisted;
} btree_node_traversed;

static bool nt_is_leaf(btree_node_traversed node) {
	return node.levels_above_leaves == 0;
}

static btree_node_traversed nt_root(btree* tree) {
	return (btree_node_traversed) {
		.persisted = tree->root,
		.levels_above_leaves = tree->levels_above_leaves
	};
}

static btree_node_traversed nt_advance(const btree_node_traversed node,
		uint64_t key) {
	for (uint8_t i = 0; i < get_n_keys(node.persisted); i++) {
		if (node.persisted->keys[i] > key) {
			return (btree_node_traversed) {
				.persisted = node.persisted->pointers[i],
				.levels_above_leaves = node.levels_above_leaves - 1,
			};
		}
	}
	return (btree_node_traversed) {
		.persisted = node.persisted->pointers[get_n_keys(node.persisted)],
		.levels_above_leaves = node.levels_above_leaves - 1,
	};
}

static void split_keys_with_preference(uint8_t total,
		enum side_preference preference,
		uint8_t *to_left, uint8_t *to_right) {
	if (preference == LEFT) {
		*to_right = total / 2;
		*to_left = total - *to_right;
		assert(*to_left >= *to_right);
	} else {
		*to_left = total / 2;
		*to_right = total - *to_left;
		assert(*to_right >= *to_left);
	}
}

void btree_init(btree* this) {
	this->root = new_empty_leaf();
	this->levels_above_leaves = 0;
}

static void destroy_recursive(btree_node_traversed node) {
	if (!nt_is_leaf(node)) {
		FOR_EACH_INTERNAL_POINTER(*node.persisted, {
			btree_node_traversed subnode = node;
			--subnode.levels_above_leaves;
			subnode.persisted = pointer;

			destroy_recursive(subnode);
		});
	}
	free(node.persisted);
}

void btree_destroy(btree* tree) {
	destroy_recursive(nt_root(tree));
	tree->root = NULL;
}

static void dump_now(btree_node_traversed node, int depth) {
	/*
	char buffer[256];
	uint64_t shift = 0;
	for (uint8_t i = 0; i < depth; i++) {
		shift += sprintf(buffer + shift, "  ");
	}
	if (node.persisted->leaf) {
		shift += sprintf(buffer + shift, "%p: ", node.persisted);
		for (uint8_t i = 0; i < get_n_keys(node.persisted); ++i) {
			shift += sprintf(buffer + shift,
					" %" PRIu64 "=%" PRIu64,
					node.persisted->keys[i],
					node.persisted->values[i]);
		}
	} else {
		shift += sprintf(buffer + shift, "%p: ", node.persisted);
		for (uint8_t i = 0; i < get_n_keys(node.persisted); ++i) {
			shift += sprintf(buffer + shift, " %p <%" PRIu64 ">",
					node.persisted->pointers[i],
					node.persisted->keys[i]);
		}
		shift += sprintf(buffer + shift, " %p",
				node.persisted->pointers[get_n_keys(node.persisted)]);
	}
	printf("%s\n", buffer);
	*/
	printf("dumping doesn't work\n");
}

static void dump_recursive(btree_node_traversed node, int depth) {
	dump_now(node, depth);
	if (!nt_is_leaf(node)) {
		FOR_EACH_INTERNAL_POINTER(*node.persisted, {
			btree_node_traversed subnode = node;
			--subnode.levels_above_leaves;
			subnode.persisted = pointer;

			dump_recursive(subnode, depth + 1);
		});
	}
}

void btree_dump(btree* this) {
	dump_recursive(nt_root(this), 0);
}

static void set_new_root(btree* this, uint64_t middle_key,
		btree_node_persisted* left, btree_node_persisted* right) {
	this->root = new_fork_node(middle_key, left, right);
}

int8_t btree_insert(btree* this, uint64_t key, uint64_t value) {
	btree_node_persisted* parent = NULL;
	btree_node_traversed node = nt_root(this);

	do {
		if ((nt_is_leaf(node) && get_n_keys(node.persisted) == LEAF_MAX_KEYS) ||
				(!nt_is_leaf(node) && get_n_keys(node.persisted) == INTERNAL_MAX_KEYS)) {
			log_verbose(1, "splitting %p", node);
			// We need to split the node now.
			btree_node_persisted* new_right_sibling =
					malloc(sizeof(btree_node_persisted));
			uint64_t middle_key;
			if (nt_is_leaf(node)) {
				split_leaf(node.persisted, new_right_sibling,
						&middle_key);
			} else {
				split_internal(node.persisted,
						new_right_sibling, &middle_key);
			}
			log_verbose(1, "middle key: %" PRIu64, middle_key);
			if (parent == NULL) {
				set_new_root(this, middle_key, node.persisted,
						new_right_sibling);
				++this->levels_above_leaves;
				IF_LOG_VERBOSE(1) {
					log_info("setting new root %p", this->root);
					log_info("I now look like this:");
					//dump_recursive(this->root, 0);
				}
			} else {
				insert_pointer(parent, middle_key,
						new_right_sibling);
			}
			if (key >= middle_key) {
				log_verbose(1, "going to right sibling (%p)",
						new_right_sibling);
				node.persisted = new_right_sibling;
			}
		}
		if (nt_is_leaf(node)) {
			break;
		}
		log_verbose(1, "now at: parent=%p node=%p",
				parent, node.persisted);
		parent = node.persisted;
		node = nt_advance(node, key);
		log_verbose(1, "went to: parent=%p node=%p",
				parent, node.persisted);
	} while (true);
	return insert_key_value_pair(node.persisted, key, value);
}

static void collapse_if_singleton_root(btree* this,
		btree_node_persisted* node) {
	if (node == this->root && get_n_keys(node) == 0) {
		// Singleton parent.
		this->root = node->pointers[0];
		free(node);
		--this->levels_above_leaves;
	}
}

int8_t btree_delete(btree* this, uint64_t key) {
	btree_node_persisted* parent = NULL;
	btree_node_traversed node = nt_root(this);

	// TODO: maybe something ultraspecial when deleting pivotal nodes?
	do {
		IF_LOG_VERBOSE(1) {
			log_info("parent:");
			//if (parent) {
				//dump_recursive(parent, 0);
			//}
			log_info("node:");
			dump_recursive(node, 1);
		}
		if (parent && ((nt_is_leaf(node) && get_n_keys(node.persisted) == LEAF_MIN_KEYS) ||
				(!nt_is_leaf(node) && get_n_keys(node.persisted) == INTERNAL_MIN_KEYS))) {
			uint8_t right_index;
			btree_node_persisted *left, *right;
			find_siblings(node.persisted, parent, &left, &right,
					&right_index);

			enum side_preference preference;
			if (left == node.persisted) {
				preference = LEFT;
			} else {
				preference = RIGHT;
			}

			if (nt_is_leaf(node)) {
				assert(left->key_count + right->key_count >=
						LEAF_MIN_KEYS);
				assert(left->key_count + right->key_count <=
						2 * LEAF_MAX_KEYS);

				if (left->key_count + right->key_count
						<= 2 * LEAF_MIN_KEYS) {
					log_verbose(1, "concatting leaves");
					append_leaf(left, right);
					node.persisted = left;
					remove_ptr_from_node(//this,
							parent, right);
					free(right);
					collapse_if_singleton_root(
							this, parent);
				} else {
					log_verbose(1, "leaf rebalance");
					uint8_t to_left, to_right;
					const uint8_t total = left->key_count + right->key_count;
					split_keys_with_preference(total,
							preference,
							&to_left, &to_right);
					rebalance_leaves(parent, left, right,
							right_index,
							to_left, to_right);
					// AKA: node = advance(parent, key);
					if (key < parent->keys[right_index - 1]) {
						node.persisted = left;
					} else {
						node.persisted = right;
					}
				}
			} else {
				assert(left->key_count + right->key_count >=
						INTERNAL_MIN_KEYS);
				assert(left->key_count + right->key_count <=
						2 * INTERNAL_MAX_KEYS);

				if (left->key_count + right->key_count
						<= 2 * INTERNAL_MIN_KEYS) {
					log_verbose(1, "concatting internals");
					append_internal(left,
							parent->keys[right_index - 1],
							right);
					node.persisted = left;
					remove_ptr_from_node(//this,
							parent, right);
					collapse_if_singleton_root(
							this, parent);
				} else {
					log_verbose(1, "internal rebalance");
					uint8_t to_left, to_right;
					const uint8_t total = left->key_count + right->key_count;
					split_keys_with_preference(total,
							preference,
							&to_left, &to_right);
					rebalance_internal(parent, left, right,
							right_index,
							to_left, to_right);
					// AKA: node = advance(parent, key);
					if (key < parent->keys[right_index - 1]) {
						node.persisted = left;
					} else {
						node.persisted = right;
					}
				}
			}

			IF_LOG_VERBOSE(1) {
				log_info("node after transforming:");
				dump_recursive(node, 0);
			}
		}
		if (nt_is_leaf(node)) {
			break;
		}
		log_verbose(1, "now at: parent=%p node=%p",
				parent, node.persisted);
		parent = node.persisted;
		node = nt_advance(node, key);
		log_verbose(1, "went to: parent=%p node=%p",
				parent, node.persisted);
	} while (true);
	assert(node.persisted == this->root ||
			get_n_keys(node.persisted) > LEAF_MIN_KEYS);
	if (!remove_from_leaf(node.persisted, key)) {
		return 1;
	}
	if (parent && get_n_keys(node.persisted) == 0) {
		remove_ptr_from_node(/*this, */parent, node.persisted);
		free(node.persisted);
		collapse_if_singleton_root(this, parent);
	} else {
		if (node.persisted != this->root) {
			assert(get_n_keys(node.persisted) >= LEAF_MIN_KEYS);
		}
	}
	return 0;
}

void btree_find(btree* this, uint64_t key, bool *found, uint64_t *value) {
	btree_node_traversed node = nt_root(this);

	while (!nt_is_leaf(node)) {
		node = nt_advance(node, key);
	}

	for (uint8_t i = 0; i < get_n_keys(node.persisted); i++) {
		if (node.persisted->keys[i] == key) {
			*found = true;
			*value = node.persisted->values[i];
			return;
		}
	}
	*found = false;
}

// Details of node representation:
static btree_node_persisted* new_empty_leaf() {
	btree_node_persisted* new_node = malloc(sizeof(btree_node_persisted));
	new_node->key_count = 0;
	return new_node;
}

static btree_node_persisted* new_fork_node(uint64_t middle_key,
		btree_node_persisted* left, btree_node_persisted* right) {
	btree_node_persisted* new_node = malloc(sizeof(btree_node_persisted));
	new_node->key_count = 1;
	new_node->keys[0] = middle_key;
	new_node->pointers[0] = left;
	new_node->pointers[1] = right;
	return new_node;
}

void split_leaf(btree_node_persisted* node,
		btree_node_persisted* new_right_sibling, uint64_t *middle_key) {
	const uint8_t to_left = node->key_count / 2;
	const uint8_t to_right = node->key_count - to_left;

	node->key_count = to_left;
	new_right_sibling->key_count = to_right;

	for (uint8_t i = 0; i < to_right; i++) {
		new_right_sibling->keys[i] = node->keys[i + to_left];
		new_right_sibling->values[i] = node->values[i + to_left];
	}

	*middle_key = new_right_sibling->keys[0];
}

void split_internal(btree_node_persisted* node,
		btree_node_persisted* new_right_sibling, uint64_t *middle_key) {
	const uint8_t to_left = node->key_count / 2;
	const uint8_t to_right = node->key_count - to_left - 1;

	assert(to_left + 1 + to_right == node->key_count);

	node->key_count = to_left;
	new_right_sibling->key_count = to_right;

	*middle_key = node->keys[to_left];
	for (uint8_t i = to_left + 1; i < to_left + 1 + to_right; i++) {
		new_right_sibling->keys[i - to_left - 1] = node->keys[i];
	}
	for (uint8_t i = 0; i < to_right + 1; i++) {
		new_right_sibling->pointers[i] =
			node->pointers[to_left + 1 + i];
	}
}

void insert_pointer(btree_node_persisted* node, uint64_t key,
		btree_node_persisted* pointer) {
	assert(node->key_count < INTERNAL_MAX_KEYS);
	uint8_t insert_at = 0xFF;
	for (uint8_t i = 0; i < node->key_count; i++) {
		if (node->keys[i] > key) {
			insert_at = i;
			break;
		}
	}
	if (insert_at == 0xFF) {
		insert_at = node->key_count;
	}
	log_verbose(2, "inserting at %" PRIu8, insert_at);
	for (uint8_t i = node->key_count; i > insert_at; --i) {
		log_verbose(2, "key %" PRIu8 " <- %" PRIu8 " (%" PRIu64 ")",
				i - 1, i, node->keys[i - 1]);
		node->keys[i] = node->keys[i - 1];
	}
	for (uint8_t i = node->key_count + 1; i > insert_at + 1; --i) {
		node->pointers[i] = node->pointers[i - 1];
	}
	node->keys[insert_at] = key;
	node->pointers[insert_at + 1] = pointer;
	++node->key_count;
}

static void remove_ptr_from_node(btree_node_persisted* parent,
		const btree_node_persisted* remove) {
	assert(parent->pointers[0] != remove);
	bool found = false;
	for (uint8_t i = 1; i < parent->key_count + 1; ++i) {
		if (!found && parent->pointers[i] == remove) {
			found = true;
		}
		if (found && i < parent->key_count) {
			parent->keys[i - 1] = parent->keys[i];
			parent->pointers[i] = parent->pointers[i + 1];
		}
	}
	assert(found);
	--parent->key_count;
}

int8_t insert_key_value_pair(btree_node_persisted* leaf,
		uint64_t key, uint64_t value) {
	assert(leaf->key_count < LEAF_MAX_KEYS);
	uint8_t insert_at = 0xFF;
	for (uint8_t i = 0; i < leaf->key_count; i++) {
		if (leaf->keys[i] == key) {
			// Duplicate keys.
			return 1;
		}
		if (leaf->keys[i] > key) {
			insert_at = i;
			break;
		}
	}
	if (insert_at == 0xFF) {
		insert_at = leaf->key_count;
	}
	for (uint8_t i = leaf->key_count; i > insert_at; --i) {
		leaf->keys[i] = leaf->keys[i - 1];
		leaf->values[i] = leaf->values[i - 1];
	}
	leaf->keys[insert_at] = key;
	leaf->values[insert_at] = value;
	++leaf->key_count;
	return 0;
}

static bool remove_from_leaf(btree_node_persisted* leaf, uint64_t key) {
	bool found = false;
	for (uint8_t i = 0; i < leaf->key_count; i++) {
		if (!found && leaf->keys[i] == key) {
			found = true;
		}
		if (found && i < leaf->key_count - 1) {
			leaf->keys[i] = leaf->keys[i + 1];
			leaf->values[i] = leaf->values[i + 1];
		}
	}
	if (!found) {
		return false;
	}
	--leaf->key_count;
	return true;
}

static void rebalance_internal(btree_node_persisted* parent,
		btree_node_persisted* left, btree_node_persisted* right,
		const uint8_t right_index,
		const uint8_t to_left, const uint8_t to_right) {
	// TODO: optimize
	const uint8_t total = left->key_count + right->key_count;

	assert(to_left >= INTERNAL_MIN_KEYS && to_right >= INTERNAL_MIN_KEYS &&
			to_left <= INTERNAL_MAX_KEYS &&
			to_right <= INTERNAL_MAX_KEYS);

	uint64_t keys[total + 1];
	btree_node_persisted* pointers[total + 2];

	for (uint8_t i = 0; i < left->key_count; i++) {
		keys[i] = left->keys[i];
	}
	keys[left->key_count] = parent->keys[right_index - 1];
	for (uint8_t i = 0; i < right->key_count; i++) {
		keys[left->key_count + 1 + i] = right->keys[i];
	}

	for (uint8_t i = 0; i < left->key_count + 1; i++) {
		pointers[i] = left->pointers[i];
	}
	for (uint8_t i = 0; i < right->key_count + 1; i++) {
		pointers[left->key_count + 1 + i] = right->pointers[i];
	}

	left->key_count = to_left;
	for (uint8_t i = 0; i < to_left; i++) {
		left->keys[i] = keys[i];
	}
	for (uint8_t i = 0; i < to_left + 1; i++) {
		left->pointers[i] = pointers[i];
	}
	log_verbose(1, "setting parent->keys[%" PRIu8 "]=%" PRIu64,
			right_index - 1, keys[to_left]);
	parent->keys[right_index - 1] = keys[to_left];
	for (uint8_t i = 0; i < to_right; i++) {
		right->keys[i] = keys[to_left + 1 + i];
	}
	for (uint8_t i = 0; i < to_right + 1; i++) {
		right->pointers[i] = pointers[to_left + 1 + i];
	}
	right->key_count = to_right;
}

static void rebalance_leaves(btree_node_persisted* parent,
		btree_node_persisted* left, btree_node_persisted* right,
		const uint8_t right_index,
		const uint8_t to_left, const uint8_t to_right) {
	// TODO: optimize
	const uint8_t total = left->key_count + right->key_count;

	assert(to_left >= LEAF_MIN_KEYS && to_right >= LEAF_MIN_KEYS &&
			to_left <= LEAF_MAX_KEYS && to_right <= LEAF_MAX_KEYS);

	uint64_t keys[total];
	uint64_t values[total];

	for (uint8_t i = 0; i < left->key_count; i++) {
		keys[i] = left->keys[i];
		values[i] = left->values[i];
	}
	for (uint8_t i = 0; i < right->key_count; i++) {
		keys[i + left->key_count] = right->keys[i];
		values[i + left->key_count] = right->values[i];
	}
	for (uint8_t i = 0; i < to_left; i++) {
		left->keys[i] = keys[i];
		left->values[i] = values[i];
	}
	left->key_count = to_left;
	for (uint8_t i = 0; i < to_right; i++) {
		right->keys[i] = keys[i + to_left];
		right->values[i] = values[i + to_left];
	}
	right->key_count = to_right;

	assert(parent->pointers[right_index - 1] == left);
	assert(parent->pointers[right_index] == right);
	parent->keys[right_index - 1] = right->keys[0];
}

static void append_internal(btree_node_persisted* target, uint64_t appended_key,
		const btree_node_persisted* source) {
	assert(target->key_count + 1 + source->key_count <= INTERNAL_MAX_KEYS);
	target->keys[target->key_count] = appended_key;
	for (uint8_t i = 0; i < source->key_count; ++i) {
		assert(target->key_count + 1 + i < INTERNAL_MAX_KEYS);
		target->keys[target->key_count + 1 + i] = source->keys[i];
	}
	for (uint8_t i = 0; i < source->key_count + 1; ++i) {
		target->pointers[target->key_count + 1 + i] =
				source->pointers[i];
	}
	target->key_count += source->key_count + 1;
}

static void append_leaf(btree_node_persisted* target,
		const btree_node_persisted* source) {
	for (uint8_t i = 0; i < source->key_count; ++i) {
		target->keys[i + target->key_count] = source->keys[i];
		target->values[i + target->key_count] = source->values[i];
	}
	target->key_count += source->key_count;
}

static uint8_t get_n_keys(const btree_node_persisted* node) {
	return node->key_count;
}

static void find_siblings(btree_node_persisted* node,
		btree_node_persisted* parent,
		btree_node_persisted** left, btree_node_persisted** right,
		uint8_t* right_index) {
	if (parent->pointers[0] == node) {
		*left = node;
		*right_index = 1;
		*right = parent->pointers[1];
	} else {
		for (uint8_t i = 1; i < parent->key_count + 1; ++i) {
			if (parent->pointers[i] == node) {
				*left = parent->pointers[i - 1];
				*right_index = i;
				*right = node;
				return;
			}
		}
		log_fatal("node not found in parent when looking for sibling");
	}
}
