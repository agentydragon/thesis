#include "traversal.h"

uint64_t hashtable_next_index(struct hashtable_data* this, uint64_t i) {
	return (i + 1) % this->blocks_size;
}
