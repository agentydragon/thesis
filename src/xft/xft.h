#ifndef XFT_XFT_H
#define XFT_XFT_H

#include "dict/dict.h"

#define BITSOF(x) (sizeof(x) * 8)

typedef uint64_t xft_key;

struct xft_node;
typedef struct xft_node xft_node;

struct xft_node {
	xft_key prefix;
	xft_node* left; // adds 0
	xft_node* right;  // adds 1
	// (values stored in leaves)

	xft_node* descendant;  // smallest leaf in right subtree (if would have no left child),
		// or largest leaf in left subtree (if would have no right child)

	// Predecessor, successor (doubly linked list of leaves)
	xft_node* prev;
	xft_node* next;
};

typedef struct {
	// For each level: hash table with nodes on that level
	//     (should use 'dynamic perfect hashing' or 'cuckoo hashing')
	//
	// Hash table: prefix => node
	dict* lss[BITSOF(xft_key) + 1];
	xft_node* root;
} xft;

void xft_init(xft*);
void xft_destroy(xft*);
bool xft_contains(xft*, xft_key key);
void xft_next(xft*, xft_key key, xft_key* next_key, bool *found);
void xft_previous(xft*, xft_key key, xft_key* previous_key, bool *found);

// Helper, exposed for test purposes.
xft_key xft_prefix(xft_key key, uint8_t length);

#endif
