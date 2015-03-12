#include "xft/xft.h"

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "log/log.h"
#include "dict/htable.h"

void xft_init(xft* this) {
	for (uint64_t i = 0; i <= BITSOF(xft_key); i++) {
		assert(!dict_init(&this->lss[i], &dict_htable, NULL));
	}
	this->root = malloc(sizeof(xft_node));
	this->root->prefix = 0;
	assert(!dict_insert(this->lss[BITSOF(xft_key) - 1], 0, (uint64_t) this->root));
}

void xft_destroy(xft* this) {
	// TODO: destroy all nodes
	for (uint64_t i = 0; i <= BITSOF(xft_key); i++) {
		dict_destroy(&this->lss[i]);
	}
	free(this->root);
	this->root = NULL;
}

bool xft_contains(xft* this, xft_key k) {
	// lss[0] == leaves
	bool found;
	dict_find(this->lss[BITSOF(xft_key)], k, NULL, &found);
	return found;
}

xft_key xft_prefix(xft_key key, uint8_t length) {
	if (length == 0) {
		return 0;
	}
	return key & ~((1LL << (uint64_t)(BITSOF(xft_key) - length)) - 1);
}

/*

static xft_node* find_lowest_ancestor(xft* this, xft_key k) {
	uint64_t min = 0, max = BITSOF(xft_key);
	while (max > min + 1) {
		uint64_t mid = (min + max) / 2;
		bool found;
		assert(!dict_find(this->lss[mid], get_prefix(k, mid), NULL, &found));
		if (found) {
			min = mid;
		} else {
			max = mid;
		}
	}
	xft_node* ancestor;
	assert(!dict_find(this->lss[min], get_prefix(k, min), &ancestor, &found));
	assert(found);
	return ancestor;
}

static xft_node* prev_leaf(xft* this, key k) {
	xft_node* ancestor = find_lowest_ancestor(this, k);
	xft_node* leaf = ancestor->descendant;
	if (leaf) {
		if (leaf->prefix > k) {
			leaf = leaf->prev;
		}
	}
	return leaf;
}

static xft_node* next_leaf(xft* this, key k) {
	xft_node* ancestor = find_lowest_ancestor(this, k);
	xft_node* leaf = ancestor->descendant;
	if (leaf) {
		if (leaf->prefix < k) {
			leaf = leaf->next;
		}
	}
	return leaf;
}

void xft_prev(xft* this, xft_key k, xft_key* sk, bool *found) {
	xft_node* leaf = next_leaf(this, k);
	if (leaf) {
		*found = true;
		*pk = leaf->prefix;
		return;
	}
	*found = false;
}

void predecessor(xft* this, xft_key k, xft_key* pk, bool *found) {
	xft_node* leaf = prev_leaf(this, k);
	if (leaf) {
		*found = true;
		*pk = leaf->prefix;
		return;
	}
	*found = false;
}
*/

/*
void insert(xft* this, xft_key key) {
	// TODO: assert not found
	xft_node* prev = prev_leaf(this, key), *next = next_leaf(this, key);
	// Create new leaf, insert into linked list.
	xft_node* my_leaf = malloc(sizeof(xft_node));
	my_leaf->prefix = key;
	my_leaf->prev = prev;
	my_leaf->next = next;

	// Walk from root, create necessary nodes on way down.
	xft_node* parent = 
	TODO
}

void delete(xft* this, xft_key key) {
	TODO
}
*/
