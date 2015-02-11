#include "btree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "../log/log.h"

// TODO: static_assert's for min-keys/max-keys conditions

typedef struct {
	btree_node_persisted* persisted;
} btree_node_traversed;

btree_node_persisted* new_empty_leaf() {
	btree_node_persisted* new_node = malloc(sizeof(btree_node_persisted));
	new_node->leaf = true;
	new_node->key_count = 0;
	return new_node;
}

btree_node_persisted* new_fork_node(uint64_t middle_key,
		btree_node_persisted* left, btree_node_persisted* right) {
	btree_node_persisted* new_node = malloc(sizeof(btree_node_persisted));
	new_node->leaf = false;
	new_node->key_count = 1;
	new_node->keys[0] = middle_key;
	new_node->pointers[0] = left;
	new_node->pointers[1] = right;
	return new_node;
}

void btree_init(btree* this) {
	this->root = new_empty_leaf();
}

btree_node_traversed TRAVERSE(btree_node_persisted* p) {
	return (btree_node_traversed) {
		.persisted = p
	};
}

static void destroy_recursive(btree_node_traversed node) {
	if (!node.persisted->leaf) {
		for (uint8_t i = 0; i < node.persisted->key_count + 1; i++) {
			destroy_recursive(TRAVERSE(
						node.persisted->pointers[i]));
		}
	}
	free(node.persisted);
}

void btree_destroy(btree* tree) {
	destroy_recursive(TRAVERSE(tree->root));
	tree->root = NULL;
}

static void dump_now(btree_node_traversed node, int depth) {
	char buffer[256];
	uint64_t shift = 0;
	for (uint8_t i = 0; i < depth; i++) {
		shift += sprintf(buffer + shift, "  ");
	}
	if (node.persisted->leaf) {
		shift += sprintf(buffer + shift, "%p: ", node.persisted);
		for (uint8_t i = 0; i < node.persisted->key_count; ++i) {
			shift += sprintf(buffer + shift,
					" %" PRIu64 "=%" PRIu64,
					node.persisted->keys[i],
					node.persisted->values[i]);
		}
	} else {
		shift += sprintf(buffer + shift, "%p: ", node.persisted);
		for (uint8_t i = 0; i < node.persisted->key_count; ++i) {
			shift += sprintf(buffer + shift, " %p <%" PRIu64 ">",
					node.persisted->pointers[i],
					node.persisted->keys[i]);
		}
		shift += sprintf(buffer + shift, " %p",
				node.persisted->pointers[node.persisted->key_count]);
	}
	printf("%s\n", buffer);
}

static void dump_recursive(btree_node_traversed node, int depth) {
	dump_now(node, depth);
	if (!node.persisted->leaf) {
		for (uint8_t i = 0; i < node.persisted->key_count + 1; ++i) {
			dump_recursive(TRAVERSE(node.persisted->pointers[i]),
					depth + 1);
		}
	}
}

void btree_dump(btree* this) {
	dump_recursive(TRAVERSE(this->root), 0);
}

void split_leaf(btree_node_persisted* node,
		btree_node_persisted* new_right_sibling, uint64_t *middle_key) {
	assert(node->leaf);
	const uint8_t to_left = node->key_count / 2;
	const uint8_t to_right = node->key_count - to_left;

	node->key_count = to_left;
	new_right_sibling->key_count = to_right;
	new_right_sibling->leaf = true;

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
	new_right_sibling->leaf = false;

	*middle_key = node->keys[to_left];
	for (uint8_t i = to_left + 1; i < to_left + 1 + to_right; i++) {
		new_right_sibling->keys[i - to_left - 1] = node->keys[i];
	}
	for (uint8_t i = 0; i < to_right + 1; i++) {
		new_right_sibling->pointers[i] =
			node->pointers[to_left + 1 + i];
	}
}

void split_node(btree_node_traversed node, btree_node_persisted* new_right_sibling,
		uint64_t *middle_key) {
	// [N2] a X b Y c Z d Q e ==> [N1] a X b <- (Y) -> [N2] c Z d Q e
	if (node.persisted->leaf) {
		split_leaf(node.persisted, new_right_sibling, middle_key);
	} else {
		split_internal(node.persisted, new_right_sibling, middle_key);
	}
}

void insert_pointer(btree_node_persisted* node, uint64_t key,
		btree_node_persisted* pointer) {
	assert(!node->leaf);
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

int8_t insert_key_value_pair(btree_node_persisted* leaf,
		uint64_t key, uint64_t value) {
	assert(leaf->leaf);
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

bool remove_from_leaf(btree_node_persisted* leaf, uint64_t key) {
	assert(leaf->leaf);
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

static void set_new_root(btree* this, uint64_t middle_key,
		btree_node_persisted* left, btree_node_persisted* right) {
	this->root = new_fork_node(middle_key, left, right);
}

btree_node_traversed advance(const btree_node_traversed node, uint64_t key) {
	for (uint8_t i = 0; i < node.persisted->key_count; i++) {
		if (node.persisted->keys[i] > key) {
			return TRAVERSE(node.persisted->pointers[i]);
		}
	}
	return TRAVERSE(node.persisted->pointers[node.persisted->key_count]);
}

int8_t btree_insert(btree* this, uint64_t key, uint64_t value) {
	btree_node_persisted* parent = NULL;
	btree_node_traversed node = TRAVERSE(this->root);

	do {
		if ((node.persisted->leaf && node.persisted->key_count == LEAF_MAX_KEYS) ||
				(!node.persisted->leaf && node.persisted->key_count == INTERNAL_MAX_KEYS)) {
			log_verbose(1, "splitting %p", node);
			// We need to split the node now.
			btree_node_persisted* new_right_sibling =
					malloc(sizeof(btree_node_persisted));
			uint64_t middle_key;
			split_node(node, new_right_sibling, &middle_key);
			log_verbose(1, "middle key: %" PRIu64, middle_key);
			if (parent == NULL) {
				set_new_root(this, middle_key, node.persisted,
						new_right_sibling);
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
				node = TRAVERSE(new_right_sibling);
			}
		}
		if (node.persisted->leaf) {
			break;
		}
		log_verbose(1, "now at: parent=%p node=%p",
				parent, node.persisted);
		parent = node.persisted;
		node = advance(node, key);
		log_verbose(1, "went to: parent=%p node=%p",
				parent, node.persisted);
	} while (true);
	return insert_key_value_pair(node.persisted, key, value);
}

enum side_preference { LEFT, RIGHT };

void rebalance_leaves(btree_node_persisted* parent,
		btree_node_persisted* left, btree_node_persisted* right,
		uint8_t right_index, enum side_preference preference) {
	assert(left->leaf && right->leaf && !parent->leaf);
	// TODO: optimize
	const uint8_t total = left->key_count + right->key_count;

	uint8_t to_left, to_right;
	if (preference == LEFT) {
		to_right = total / 2;
		to_left = total - to_right;
		assert(to_left >= to_right);
	} else {
		to_left = total / 2;
		to_right = total - to_left;
		assert(to_right >= to_left);
	}

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

void rebalance_internal(btree_node_persisted* parent,
		btree_node_persisted* left, btree_node_persisted* right,
		uint8_t right_index, enum side_preference preference) {
	assert(!left->leaf && !right->leaf && !parent->leaf);

	// TODO: optimize
	const uint8_t total = left->key_count + right->key_count;

	uint8_t to_left, to_right;
	if (preference == LEFT) {
		to_right = total / 2;
		to_left = total - to_right;
		assert(to_left >= to_right);
	} else {
		to_left = total / 2;
		to_right = total - to_left;
		assert(to_right >= to_left);
	}

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

void find_siblings(btree_node_persisted* node, btree_node_persisted* parent,
		btree_node_persisted** left, btree_node_persisted** right,
		uint8_t* right_index) {
	assert(!parent->leaf);
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

void append_leaves(btree_node_persisted* target,
		const btree_node_persisted* source) {
	for (uint8_t i = 0; i < source->key_count; ++i) {
		target->keys[i + target->key_count] = source->keys[i];
		target->values[i + target->key_count] = source->values[i];
	}
	target->key_count += source->key_count;
}

void append_internal(btree_node_persisted* target, uint64_t appended_key,
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

void remove_ptr_from_node(/*btree* tree, */btree_node_persisted* parent,
		const btree_node_persisted* remove) {
	assert(!parent->leaf);
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
	/*
	if (parent != tree->root) {
		assert(parent->key_count >= INTERNAL_MIN_KEYS);
	}
	*/
}

static void collapse_if_singleton_root(btree* this,
		btree_node_persisted* node) {
	if (node == this->root && node->key_count == 0) {
		// Singleton parent.
		this->root = node->pointers[0];
		free(node);
	}
}

int8_t btree_delete(btree* this, uint64_t key) {
	btree_node_persisted* parent = NULL;
	btree_node_traversed node = TRAVERSE(this->root);

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
		if (parent && ((node.persisted->leaf && node.persisted->key_count == LEAF_MIN_KEYS) ||
				(!node.persisted->leaf && node.persisted->key_count == INTERNAL_MIN_KEYS))) {
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

			assert(left->leaf == right->leaf);
			if (left->leaf) {
				assert(left->key_count + right->key_count >=
						LEAF_MIN_KEYS);
				assert(left->key_count + right->key_count <=
						2 * LEAF_MAX_KEYS);

				if (left->key_count + right->key_count
						<= 2 * LEAF_MIN_KEYS) {
					log_verbose(1, "concatting leaves");
					append_leaves(left, right);
					node = TRAVERSE(left);
					remove_ptr_from_node(//this,
							parent, right);
					free(right);
					collapse_if_singleton_root(
							this, parent);
				} else {
					log_verbose(1, "leaf rebalance");
					rebalance_leaves(parent, left, right,
							right_index,
							preference);
					// AKA: node = advance(parent, key);
					if (key < parent->keys[right_index - 1]) {
						node = TRAVERSE(left);
					} else {
						node = TRAVERSE(right);
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
					node = TRAVERSE(left);
					remove_ptr_from_node(//this,
							parent, right);
					collapse_if_singleton_root(
							this, parent);
				} else {
					log_verbose(1, "internal rebalance");
					rebalance_internal(parent, left, right,
							right_index,
							preference);
					// AKA: node = advance(parent, key);
					if (key < parent->keys[right_index - 1]) {
						node = TRAVERSE(left);
					} else {
						node = TRAVERSE(right);
					}
				}
			}

			IF_LOG_VERBOSE(1) {
				log_info("node after transforming:");
				dump_recursive(node, 0);
			}
		}
		if (node.persisted->leaf) {
			break;
		}
		log_verbose(1, "now at: parent=%p node=%p",
				parent, node.persisted);
		parent = node.persisted;
		node = advance(node, key);
		log_verbose(1, "went to: parent=%p node=%p",
				parent, node.persisted);
	} while (true);
	assert(node.persisted->leaf);
	assert(node.persisted == this->root ||
			node.persisted->key_count > LEAF_MIN_KEYS);
	if (!remove_from_leaf(node.persisted, key)) {
		return 1;
	}
	if (parent && node.persisted->key_count == 0) {
		remove_ptr_from_node(/*this, */parent, node.persisted);
		free(node.persisted);
		collapse_if_singleton_root(this, parent);
	} else {
		if (node.persisted != this->root) {
			assert(node.persisted->key_count >= LEAF_MIN_KEYS);
		}
	}
	return 0;
}

void btree_find(btree* this, uint64_t key, bool *found, uint64_t *value) {
	btree_node_traversed node = TRAVERSE(this->root);

	while (!node.persisted->leaf) {
		node = advance(node, key);
	}

	for (uint8_t i = 0; i < node.persisted->key_count; i++) {
		if (node.persisted->keys[i] == key) {
			*found = true;
			*value = node.persisted->values[i];
			return;
		}
	}
	*found = false;
}
