#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>

#define log_plain(fmt,...) do { \
	fprintf(stderr, "     : " fmt "\n", ##__VA_ARGS__); \
} while (0)

#ifndef NO_LOG_INFO
#define log_info(fmt,...) do { \
	fprintf(stderr, " INFO: " fmt "\n", ##__VA_ARGS__); \
} while (0)
#else
#define log_info(...) (void)(0)
#endif

#define log_error(fmt,...) do { \
	fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__); \
} while (0)

#define log_fatal(...) do { \
	log_error(__VA_ARGS__); \
	abort(); \
} while (0)

#define CHECK(x) do { \
	if (!x) { \
		log_error("check failed: " #x); \
		abort(); \
	} \
} while (0)

#endif
