#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

void __log_basic(const char* tag, const char* format, ...)
		__attribute__((format (printf, 2, 3)));
void log_fatal(const char* format, ...) __attribute__((noreturn));
// TODO: log_q(uiet)fatal?

#define CHECK(assertion,message,...) do { \
	if (!(assertion)) { \
		log_fatal(message, ##__VA_ARGS__); \
	} \
} while (0)

#define log_plain(fmt,...) do { \
	__log_basic(" ", fmt, ##__VA_ARGS__); \
} while (0)

#ifndef NO_LOG_INFO
#define log_info(fmt,...) do { \
	__log_basic("i", fmt, ##__VA_ARGS__); \
} while (0)
#else
#define log_info(...) (void)(0)
#endif

#define log_error(fmt,...) do { \
	__log_basic("E", fmt, ##__VA_ARGS__); \
} while (0)

#ifndef LOG_VERBOSITY
#define LOG_VERBOSITY 0
#endif

#define IF_LOG_VERBOSE(level) if (LOG_VERBOSITY >= level)

// TODO: other header
#define log_verbose(level,fmt,...) do { \
	if (LOG_VERBOSITY >= level) { \
		__log_basic(" INFO", fmt, ##__VA_ARGS__); \
	} \
} while (0)

#include "util/likeliness.h"

// A bit like assert(x), but evaluated even with NDEBUG.
#ifdef NDEBUG
#define ASSERT(x) do { \
	if (unlikely(!(x))) { \
		log_fatal("ASSERT(%s) failed: %s:%d", #x, __FILE__, __LINE__); \
	} \
} while (0)
#else
#define ASSERT(x) assert(x)
#endif

#endif
