#ifndef PERFORMANCE_RANDOM_READ_INCLUDED
#define PERFORMANCE_RANDOM_READ_INCLUDED

#include "dict/dict.h"

void time_random_reads(const dict_api* api, int size, int reads);

#endif
