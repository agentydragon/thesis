#ifndef KSPLAY_KSPLAY_H
#define KSPLAY_KSPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define KSPLAY_K 3

// This includes a +1 buffer pair, because nodes may temporarily overflow
// between inserting and restoring balance.
// TODO: Remove the buffer, it's wasting space.
#define KSPLAY_MAX_NODE_KEYS KSPLAY_K
#define KSPLAY_MAX_NODE_CHILDREN (KSPLAY_K + 1)

struct ksplay_node;

typedef struct {
	uint64_t key;
	uint64_t value;
} ksplay_pair;

struct ksplay_node {
	uint8_t key_count;
	ksplay_pair pairs[KSPLAY_MAX_NODE_KEYS];
	struct ksplay_node* children[KSPLAY_MAX_NODE_CHILDREN + 1];
};

typedef struct ksplay_node ksplay_node;

typedef struct {
	ksplay_node* root;
} ksplay;

void ksplay_init(ksplay* this);
void ksplay_destroy(ksplay* this);

int8_t ksplay_insert(ksplay* this, uint64_t key, uint64_t value);
int8_t ksplay_delete(ksplay* this, uint64_t key);
void ksplay_find(ksplay* this, uint64_t key, uint64_t *value, bool *found);
// TODO: find_next, find_previous

typedef struct {
	ksplay_node** nodes;
	uint64_t count;
	uint64_t capacity;
} ksplay_node_buffer;

// Internal methods, exposed for testing.
ksplay_node* ksplay_compose(ksplay_pair* pairs, ksplay_node** children,
		uint64_t key_count);
ksplay_node_buffer ksplay_walk_to(ksplay* this, uint64_t key);

#endif
