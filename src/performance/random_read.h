#ifndef PERFORMANCE_RANDOM_READ_INCLUDED
#define PERFORMANCE_RANDOM_READ_INCLUDED

#include "hash/hash.h"

void time_random_reads(const hash_api* api, int size, int reads);

#endif
