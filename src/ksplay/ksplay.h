#ifndef KSPLAY_KSPLAY_H
#define KSPLAY_KSPLAY_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define KSPLAY_K 3
#define KSPLAY_MAX_EXPLORE_KEYS ((KSPLAY_K + 2) * (KSPLAY_K - 1) + 1)

// This includes a +1 buffer pair, because nodes may temporarily overflow
// between inserting and restoring balance.
// TODO: Remove the buffer, it's wasting space.
#define KSPLAY_MAX_NODE_KEYS KSPLAY_K
#define KSPLAY_MAX_NODE_CHILDREN (KSPLAY_K + 1)

struct ksplay_node;

struct {
	uint64_t ksplay_steps;
	uint64_t composed_keys;
} KSPLAY_COUNTERS;

typedef struct {
	uint64_t key;
	uint64_t value;
} ksplay_pair;

// Stores pairs and children. Keys are in sorted order.
// The number of stored keys is implicit - missing keys are at the end
// and they are set to UINT64_MAX.
struct ksplay_node {
	ksplay_pair pairs[KSPLAY_MAX_NODE_KEYS];
	struct ksplay_node* children[KSPLAY_MAX_NODE_CHILDREN + 1];
};

typedef struct ksplay_node ksplay_node;

typedef struct {
	uint64_t size;
	ksplay_node* root;
} ksplay;

void ksplay_init(ksplay* this);
void ksplay_destroy(ksplay* this);

bool ksplay_insert(ksplay* this, uint64_t key, uint64_t value);
bool ksplay_delete(ksplay* this, uint64_t key);
bool ksplay_find(ksplay* this, uint64_t key, uint64_t *value);
bool ksplay_next_key(ksplay* this, uint64_t key, uint64_t* next_key);
bool ksplay_previous_key(ksplay* this, uint64_t key, uint64_t* previous_key);

void ksplay_dump(ksplay* tree);

// Internal methods, exposed for testing.
typedef struct {
	ksplay_node** nodes;
	uint64_t count;
	uint64_t capacity;
} ksplay_node_buffer;

// Select an allocation method for the stack.
// STATIC is hacky, but much faster.
// #define KSPLAY_STACK_MALLOC
#define KSPLAY_STACK_STATIC

ksplay_node_buffer ksplay_walk_to(ksplay* this, uint64_t key);
void ksplay_flatten(ksplay_node_buffer* stack, ksplay_pair* _pairs,
		ksplay_node** _children, uint64_t* key_count);

typedef struct {
	uint64_t remaining;
	ksplay_node** nodes;
} ksplay_node_pool;

ksplay_node* ksplay_compose(ksplay_node_pool* pool,
		ksplay_pair* pairs, ksplay_node** children, uint64_t key_count);
ksplay_node* ksplay_split_overfull(ksplay_node* root);
void ksplay_dump_dot(const ksplay* this, FILE* output);
uint8_t ksplay_node_key_count(ksplay_node* x);
void ksplay_check_invariants(const ksplay* this);

#endif
