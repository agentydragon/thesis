#include "btree/btree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "log/log.h"

// TODO: static_assert's for min-keys/max-keys conditions

// TODO: Travis is being a dinosaur
// static_assert(sizeof(btree_node_persisted) == 64,
// 		"btree_node_persisted not aligned on cache line");

// Details of node representation:
// TODO: document empty slots; TODO: trick to represent SLOT_UNUSED=SLOT_UNUSED

// Equal to dict DICT_RESERVED_KEY
#define SLOT_UNUSED 0xFFFFFFFFFFFFFFFF

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
static void rebalance_leaves(
		btree_node_persisted* left, btree_node_persisted* right,
		const uint8_t to_left, const uint8_t to_right,
		uint64_t* right_min_key);
static void append_internal(btree_node_persisted* target, uint64_t appended_key,
		const btree_node_persisted* source);
static void append_leaf(btree_node_persisted* target,
		btree_node_persisted* source);
static uint8_t get_n_internal_keys(const btree_node_persisted* node);
static void find_siblings(btree_node_persisted* node,
		btree_node_persisted* parent,
		btree_node_persisted** left, btree_node_persisted** right,
		uint8_t* right_index);
#define FOR_EACH_INTERNAL_POINTER(persisted_node,code) do { \
	for (uint8_t _i = 0; _i < get_n_internal_keys(persisted_node) + 1; ++_i) { \
		btree_node_persisted* const pointer = persisted_node->internal.pointers[_i]; \
		code \
	} \
} while (0)

// B-tree "meta-algorithm":
enum side_preference { LEFT, RIGHT };

bool nt_is_leaf(btree_node_traversed node) {
	return node.levels_above_leaves == 0;
}

btree_node_traversed nt_root(btree* tree) {
	return (btree_node_traversed) {
		.persisted = tree->root,
		.levels_above_leaves = tree->levels_above_leaves
	};
}

static btree_node_traversed nt_advance(const btree_node_traversed node,
		uint64_t key) {
	for (uint8_t i = 0; i < get_n_internal_keys(node.persisted); i++) {
		if (node.persisted->internal.keys[i] > key) {
			return (btree_node_traversed) {
				.persisted = node.persisted->internal.pointers[i],
				.levels_above_leaves = node.levels_above_leaves - 1,
			};
		}
	}
	return (btree_node_traversed) {
		.persisted = node.persisted->internal.pointers[get_n_internal_keys(node.persisted)],
		.levels_above_leaves = node.levels_above_leaves - 1,
	};
}

static void split_keys_with_preference(uint8_t total_keys,
		enum side_preference preference,
		uint8_t *to_left, uint8_t *to_right) {
	if (preference == LEFT) {
		*to_right = total_keys / 2;
		*to_left = total_keys - *to_right;
		assert(*to_left >= *to_right);
	} else {
		*to_left = total_keys / 2;
		*to_right = total_keys - *to_left;
		assert(*to_right >= *to_left);
	}
}

void btree_init(btree* this) {
	this->root = new_empty_leaf();
	this->levels_above_leaves = 0;
}

static void destroy_recursive(btree_node_traversed node) {
	if (!nt_is_leaf(node)) {
		FOR_EACH_INTERNAL_POINTER(node.persisted, {
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

static void set_new_root(btree* this, uint64_t middle_key,
		btree_node_persisted* left, btree_node_persisted* right) {
	this->root = new_fork_node(middle_key, left, right);
}

int8_t btree_insert(btree* this, uint64_t key, uint64_t value) {
	if (key == SLOT_UNUSED && value == SLOT_UNUSED) {
		log_fatal("Attempted to insert reserved value.");
	}

	btree_node_persisted* parent = NULL;
	btree_node_traversed node = nt_root(this);

	do {
		if ((nt_is_leaf(node) && get_n_leaf_keys(node.persisted) == LEAF_MAX_KEYS) ||
				(!nt_is_leaf(node) && get_n_internal_keys(node.persisted) == INTERNAL_MAX_KEYS)) {
			log_verbose(1, "splitting %p", node.persisted);
			// We need to split the node now.
			btree_node_persisted* new_right_sibling;
			uint64_t middle_key;
			if (nt_is_leaf(node)) {
				new_right_sibling = new_empty_leaf();
				split_leaf(node.persisted, new_right_sibling,
						&middle_key);
			} else {
				new_right_sibling = malloc(sizeof(btree_node_persisted));
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
	if (node == this->root && get_n_internal_keys(node) == 0) {
		// Singleton parent.
		this->root = node->internal.pointers[0];
		free(node);
		--this->levels_above_leaves;
	}
}

int8_t btree_delete(btree* this, uint64_t key) {
	btree_node_persisted* parent = NULL;
	btree_node_traversed node = nt_root(this);

	// TODO: maybe something ultraspecial when deleting pivotal nodes?
	do {
		if (parent && ((nt_is_leaf(node) && get_n_leaf_keys(node.persisted) == LEAF_MIN_KEYS) ||
				(!nt_is_leaf(node) && get_n_internal_keys(node.persisted) == INTERNAL_MIN_KEYS))) {
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

			uint8_t total_keys;
			if (nt_is_leaf(node)) {
				total_keys = get_n_leaf_keys(left) + get_n_leaf_keys(right);
				assert(total_keys >= LEAF_MIN_KEYS);
				assert(total_keys <= 2 * LEAF_MAX_KEYS);

				if (total_keys <= 2 * LEAF_MIN_KEYS) {
					log_verbose(1, "concatting leaves");
					append_leaf(left, right);
					node.persisted = left;
					remove_ptr_from_node(parent, right);
					free(right);
					collapse_if_singleton_root(
							this, parent);
				} else {
					log_verbose(1, "leaf rebalance");
					uint8_t to_left, to_right;
					split_keys_with_preference(total_keys,
							preference,
							&to_left, &to_right);

					uint64_t right_min_key;
					assert(to_left >= LEAF_MIN_KEYS &&
							to_right >= LEAF_MIN_KEYS &&
							to_left <= LEAF_MAX_KEYS &&
							to_right <= LEAF_MAX_KEYS);
					rebalance_leaves(left, right,
							to_left, to_right,
							&right_min_key);

					//assert(parent->internal.pointers[right_index - 1] == left);
					//assert(parent->internal.pointers[right_index] == right);
					parent->internal.keys[right_index - 1] = right_min_key;

					// AKA: node = advance(parent, key);
					if (key < parent->internal.keys[right_index - 1]) {
						node.persisted = left;
					} else {
						node.persisted = right;
					}
				}
			} else {
				total_keys = get_n_internal_keys(left) + get_n_internal_keys(right);
				assert(total_keys >= INTERNAL_MIN_KEYS);
				assert(total_keys <= 2 * INTERNAL_MAX_KEYS);

				if (total_keys <= 2 * INTERNAL_MIN_KEYS) {
					log_verbose(1, "concatting internals");
					append_internal(left,
							parent->internal.keys[right_index - 1],
							right);
					node.persisted = left;
					remove_ptr_from_node(parent, right);
					free(right);
					collapse_if_singleton_root(
							this, parent);
				} else {
					log_verbose(1, "internal rebalance");
					uint8_t to_left, to_right;
					split_keys_with_preference(total_keys,
							preference,
							&to_left, &to_right);
					rebalance_internal(parent, left, right,
							right_index,
							to_left, to_right);
					// AKA: node = advance(parent, key);
					if (key < parent->internal.keys[right_index - 1]) {
						node.persisted = left;
					} else {
						node.persisted = right;
					}
				}
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
			get_n_leaf_keys(node.persisted) > LEAF_MIN_KEYS);
	if (!remove_from_leaf(node.persisted, key)) {
		return 1;
	}
	if (parent && get_n_leaf_keys(node.persisted) == 0) {
		remove_ptr_from_node(parent, node.persisted);
		free(node.persisted);
		collapse_if_singleton_root(this, parent);
	} else {
		if (node.persisted != this->root) {
			assert(get_n_leaf_keys(node.persisted) >= LEAF_MIN_KEYS);
		}
	}
	return 0;
}

void btree_find(btree* this, uint64_t key, uint64_t *value, bool *found) {
	btree_node_traversed node = nt_root(this);

	while (!nt_is_leaf(node)) {
		node = nt_advance(node, key);
	}

	for (uint8_t i = 0; i < get_n_leaf_keys(node.persisted); i++) {
		if (node.persisted->leaf.keys[i] == key) {
			*found = true;
			if (value) {
				*value = node.persisted->leaf.values[i];
			}
			return;
		}
	}
	*found = false;
}

// Details of node representation:
static void clear_leaf(btree_node_persisted* leaf) {
	for (uint8_t i = 0; i < LEAF_MAX_KEYS; ++i) {
		leaf->leaf.keys[i] = SLOT_UNUSED;
		leaf->leaf.values[i] = SLOT_UNUSED;
	}
}

static btree_node_persisted* new_empty_leaf() {
	btree_node_persisted* new_node = malloc(sizeof(btree_node_persisted));
	clear_leaf(new_node);
	return new_node;
}

static btree_node_persisted* new_fork_node(uint64_t middle_key,
		btree_node_persisted* left, btree_node_persisted* right) {
	btree_node_persisted* new_node = malloc(sizeof(btree_node_persisted));
	new_node->internal.key_count = 1;
	new_node->internal.keys[0] = middle_key;
	new_node->internal.pointers[0] = left;
	new_node->internal.pointers[1] = right;
	return new_node;
}

void split_leaf(btree_node_persisted* node,
		btree_node_persisted* new_right_sibling, uint64_t *middle_key) {
	assert(get_n_leaf_keys(new_right_sibling) == 0);

	const uint8_t total_keys = get_n_leaf_keys(node);
	const uint8_t to_left = total_keys / 2;
	const uint8_t to_right = total_keys - to_left;
	assert(to_left >= LEAF_MIN_KEYS && to_right >= LEAF_MIN_KEYS &&
			to_left <= LEAF_MAX_KEYS && to_right <= LEAF_MAX_KEYS);
	rebalance_leaves(node, new_right_sibling, to_left, to_right, middle_key);
}

void split_internal(btree_node_persisted* node,
		btree_node_persisted* new_right_sibling, uint64_t *middle_key) {
	const uint8_t to_left = node->internal.key_count / 2;
	const uint8_t to_right = node->internal.key_count - to_left - 1;

	assert(to_left + 1 + to_right == node->internal.key_count);

	node->internal.key_count = to_left;
	new_right_sibling->internal.key_count = to_right;

	*middle_key = node->internal.keys[to_left];
	for (uint8_t i = to_left + 1; i < to_left + 1 + to_right; i++) {
		new_right_sibling->internal.keys[i - to_left - 1] = node->internal.keys[i];
	}
	for (uint8_t i = 0; i < to_right + 1; i++) {
		new_right_sibling->internal.pointers[i] =
			node->internal.pointers[to_left + 1 + i];
	}
}

void insert_pointer(btree_node_persisted* node, uint64_t key,
		btree_node_persisted* pointer) {
	assert(node->internal.key_count < INTERNAL_MAX_KEYS);
	uint8_t insert_at = 0xFF;
	for (uint8_t i = 0; i < node->internal.key_count; i++) {
		if (node->internal.keys[i] > key) {
			insert_at = i;
			break;
		}
	}
	if (insert_at == 0xFF) {
		insert_at = node->internal.key_count;
	}
	log_verbose(2, "inserting at %" PRIu8, insert_at);
	for (uint8_t i = node->internal.key_count; i > insert_at; --i) {
		log_verbose(2, "key %" PRIu8 " <- %" PRIu8 " (%" PRIu64 ")",
				i - 1, i, node->internal.keys[i - 1]);
		node->internal.keys[i] = node->internal.keys[i - 1];
	}
	for (uint8_t i = node->internal.key_count + 1; i > insert_at + 1; --i) {
		node->internal.pointers[i] = node->internal.pointers[i - 1];
	}
	node->internal.keys[insert_at] = key;
	node->internal.pointers[insert_at + 1] = pointer;
	++node->internal.key_count;
}

static void remove_ptr_from_node(btree_node_persisted* parent,
		const btree_node_persisted* remove) {
	assert(parent->internal.pointers[0] != remove);
	bool found = false;
	for (uint8_t i = 1; i < parent->internal.key_count + 1; ++i) {
		if (!found && parent->internal.pointers[i] == remove) {
			found = true;
		}
		if (found && i < parent->internal.key_count) {
			parent->internal.keys[i - 1] = parent->internal.keys[i];
			parent->internal.pointers[i] = parent->internal.pointers[i + 1];
		}
	}
	assert(found);
	--parent->internal.key_count;
}

int8_t insert_key_value_pair(btree_node_persisted* leaf,
		uint64_t key, uint64_t value) {
	assert(get_n_leaf_keys(leaf) < LEAF_MAX_KEYS);
	uint8_t insert_at;
	for (insert_at = 0; insert_at < LEAF_MAX_KEYS; insert_at++) {
		if (leaf->leaf.keys[insert_at] == SLOT_UNUSED &&
				leaf->leaf.values[insert_at] == SLOT_UNUSED) {
			break;
		}
		if (leaf->leaf.keys[insert_at] == key) {
			// Duplicate keys.
			return 1;
		}
		if (leaf->leaf.keys[insert_at] > key) {
			break;
		}
	}
	for (uint8_t i = get_n_leaf_keys(leaf); i > insert_at; --i) {
		leaf->leaf.keys[i] = leaf->leaf.keys[i - 1];
		leaf->leaf.values[i] = leaf->leaf.values[i - 1];
	}
	leaf->leaf.keys[insert_at] = key;
	leaf->leaf.values[insert_at] = value;
	return 0;
}

static bool remove_from_leaf(btree_node_persisted* leaf, uint64_t key) {
	bool found = false;
	for (uint8_t i = 0; i < LEAF_MAX_KEYS; i++) {
		if (leaf->leaf.keys[i] == SLOT_UNUSED &&
				leaf->leaf.values[i] == SLOT_UNUSED) {
			break;
		}
		if (!found && leaf->leaf.keys[i] == key) {
			found = true;
		}
		if (found && i < LEAF_MAX_KEYS - 1) {
			leaf->leaf.keys[i] = leaf->leaf.keys[i + 1];
			leaf->leaf.values[i] = leaf->leaf.values[i + 1];
		}
	}
	if (found) {
		leaf->leaf.keys[LEAF_MAX_KEYS - 1] = SLOT_UNUSED;
		leaf->leaf.values[LEAF_MAX_KEYS - 1] = SLOT_UNUSED;
	}
	return found;
}

static void rebalance_internal(btree_node_persisted* parent,
		btree_node_persisted* left, btree_node_persisted* right,
		const uint8_t right_index,
		const uint8_t to_left, const uint8_t to_right) {
	// TODO: optimize
	const uint8_t total_keys = left->internal.key_count + right->internal.key_count;

	assert(to_left >= INTERNAL_MIN_KEYS && to_right >= INTERNAL_MIN_KEYS &&
			to_left <= INTERNAL_MAX_KEYS &&
			to_right <= INTERNAL_MAX_KEYS);

	uint64_t keys[total_keys + 1];
	btree_node_persisted* pointers[total_keys + 2];

	for (uint8_t i = 0; i < left->internal.key_count; i++) {
		keys[i] = left->internal.keys[i];
	}
	keys[left->internal.key_count] = parent->internal.keys[right_index - 1];
	for (uint8_t i = 0; i < right->internal.key_count; i++) {
		keys[left->internal.key_count + 1 + i] = right->internal.keys[i];
	}

	for (uint8_t i = 0; i < left->internal.key_count + 1; i++) {
		pointers[i] = left->internal.pointers[i];
	}
	for (uint8_t i = 0; i < right->internal.key_count + 1; i++) {
		pointers[left->internal.key_count + 1 + i] = right->internal.pointers[i];
	}

	left->internal.key_count = to_left;
	for (uint8_t i = 0; i < to_left; i++) {
		left->internal.keys[i] = keys[i];
	}
	for (uint8_t i = 0; i < to_left + 1; i++) {
		left->internal.pointers[i] = pointers[i];
	}
	log_verbose(1, "setting parent->keys[%" PRIu8 "]=%" PRIu64,
			right_index - 1, keys[to_left]);
	parent->internal.keys[right_index - 1] = keys[to_left];
	for (uint8_t i = 0; i < to_right; i++) {
		right->internal.keys[i] = keys[to_left + 1 + i];
	}
	for (uint8_t i = 0; i < to_right + 1; i++) {
		right->internal.pointers[i] = pointers[to_left + 1 + i];
	}
	right->internal.key_count = to_right;
}

static void rebalance_leaves(
		btree_node_persisted* left, btree_node_persisted* right,
		const uint8_t to_left, const uint8_t to_right,
		uint64_t* right_min_key) {
	// TODO: optimize
	const uint8_t total_keys = get_n_leaf_keys(left) + get_n_leaf_keys(right);

	uint64_t keys[total_keys];
	uint64_t values[total_keys];

	const uint8_t left_keys = get_n_leaf_keys(left);
	const uint8_t right_keys = get_n_leaf_keys(right);

	for (uint8_t i = 0; i < left_keys; i++) {
		keys[i] = left->leaf.keys[i];
		values[i] = left->leaf.values[i];
	}
	for (uint8_t i = 0; i < right_keys; i++) {
		keys[i + left_keys] = right->leaf.keys[i];
		values[i + left_keys] = right->leaf.values[i];
	}
	clear_leaf(left);
	for (uint8_t i = 0; i < to_left; i++) {
		left->leaf.keys[i] = keys[i];
		left->leaf.values[i] = values[i];
	}
	clear_leaf(right);
	for (uint8_t i = 0; i < to_right; i++) {
		right->leaf.keys[i] = keys[i + to_left];
		right->leaf.values[i] = values[i + to_left];
	}
	if (right_min_key != NULL) {
		*right_min_key = right->leaf.keys[0];
	}
}

static void append_internal(btree_node_persisted* target, uint64_t appended_key,
		const btree_node_persisted* source) {
	assert(target->internal.key_count + 1 + source->internal.key_count <= INTERNAL_MAX_KEYS);
	target->internal.keys[target->internal.key_count] = appended_key;
	for (uint8_t i = 0; i < source->internal.key_count; ++i) {
		assert(target->internal.key_count + 1 + i < INTERNAL_MAX_KEYS);
		target->internal.keys[target->internal.key_count + 1 + i] =
				source->internal.keys[i];
	}
	for (uint8_t i = 0; i < source->internal.key_count + 1; ++i) {
		target->internal.pointers[target->internal.key_count + 1 + i] =
				source->internal.pointers[i];
	}
	target->internal.key_count += source->internal.key_count + 1;
}

static void append_leaf(btree_node_persisted* target,
		btree_node_persisted* source) {
	const uint8_t total_keys = get_n_leaf_keys(source) + get_n_leaf_keys(target);
	assert(total_keys >= LEAF_MIN_KEYS && total_keys <= LEAF_MAX_KEYS);
	rebalance_leaves(target, source, total_keys, 0, NULL);
}

uint8_t get_n_leaf_keys(const btree_node_persisted* node) {
	for (uint8_t i = 0; i < LEAF_MAX_KEYS; i++) {
		if (node->leaf.keys[i] == SLOT_UNUSED &&
				node->leaf.values[i] == SLOT_UNUSED) {
			return i;
		}
	}
	return LEAF_MAX_KEYS;
}

static uint8_t get_n_internal_keys(const btree_node_persisted* node) {
	return node->internal.key_count;
}

static void find_siblings(btree_node_persisted* node,
		btree_node_persisted* parent,
		btree_node_persisted** left, btree_node_persisted** right,
		uint8_t* right_index) {
	if (parent->internal.pointers[0] == node) {
		*left = node;
		*right_index = 1;
		*right = parent->internal.pointers[1];
	} else {
		for (uint8_t i = 1; i < get_n_internal_keys(parent) + 1; ++i) {
			if (parent->internal.pointers[i] == node) {
				*left = parent->internal.pointers[i - 1];
				*right_index = i;
				*right = node;
				return;
			}
		}
		log_fatal("node not found in parent when looking for sibling");
	}
}

#define BTREE_DOT_POINTERS_IN_LABELS false
#define BTREE_DOT_VALUES_IN_LABELS false

static void _dump_dot(btree_node_traversed node, FILE* output) {
	fprintf(output, "    node%p[label = \"", node.persisted);

	if (BTREE_DOT_POINTERS_IN_LABELS) {
		fprintf(output, "{%p|", node.persisted);
	} else {
		fprintf(output, "{");
	}
	fprintf(output, "{");
	if (nt_is_leaf(node)) {
		for (uint8_t i = 0; i < get_n_leaf_keys(node.persisted); ++i) {
			if (i != 0) {
				fprintf(output, "|");
			}
			fprintf(output, "%" PRIu64,
					node.persisted->leaf.keys[i]);
			if (BTREE_DOT_VALUES_IN_LABELS) {
				fprintf(output, "=%" PRIu64,
					node.persisted->leaf.values[i]);
			}
		}
	} else {
		for (uint8_t i = 0; i < get_n_internal_keys(node.persisted);
				++i) {
			fprintf(output, "<child%d>|%" PRIu64 "|",
					i, node.persisted->internal.keys[i]);
		}
		fprintf(output, "<child%d>",
				get_n_internal_keys(node.persisted));
	}
	fprintf(output, "}}\"];\n");

	if (!nt_is_leaf(node)) {
		for (uint8_t i = 0; i <= get_n_internal_keys(node.persisted);
				++i) {
			fprintf(output, "    \"node%p\":child%d -> \"node%p\";\n",
					node.persisted, i, node.persisted->internal.pointers[i]);
		}

		for (uint8_t i = 0; i <= get_n_internal_keys(node.persisted);
				++i) {
			btree_node_traversed child = {
				.persisted = node.persisted->internal.pointers[i],
				.levels_above_leaves = node.levels_above_leaves - 1
			};
			_dump_dot(child, output);
		}
	}
}

void btree_dump_dot(const btree* this, FILE* output) {
	fprintf(output, "digraph btree_%p {\n", this);
	fprintf(output, "    splines = polyline;\n");
	fprintf(output, "    node [shape = record, height = .1];\n");
	// TODO: constness trouble and all, can't use nt_root here :(
	btree_node_traversed root = {
		.persisted = this->root,
		.levels_above_leaves = this->levels_above_leaves
	};
	_dump_dot(root, output);
	fprintf(output, "}\n");
}
