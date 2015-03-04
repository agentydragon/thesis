#include "util/human.h"

#include <stdlib.h>

#include "log/log.h"

uint64_t parse_human_i(char* arg) {
	char* end;
	uint64_t base = strtol(arg, &end, 10);
	uint64_t multiplier = 0;

	switch (*end) {
	case '\0':
		multiplier = 1;
		break;
	case 'k':
		++end;
		multiplier = 1024;
		break;
	case 'M':
		++end;
		multiplier = 1024 * 1024;
		break;
	}
	if (multiplier == 0 || *end != '\0') {
		log_fatal("invalid number: %s", arg);
	}

	return multiplier * base;
}

double parse_human_d(char* arg) {
	// TODO: error handling
	return atof(arg);
}
