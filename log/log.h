#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <stdio.h>

#define log_error(fmt,...) do { \
	fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__); \
} while (0)

#define log_fatal(...) do { \
	log_error(__VA_ARGS__); \
	abort(); \
} while (0)

#endif
