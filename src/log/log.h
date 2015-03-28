#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <stdlib.h>

void __log_basic(const char* tag, const char* func, const char* file, int line,
		const char* format, ...) __attribute__((format (printf, 5, 6)));
void __log_backtrace(int stripped_levels);
// TODO: log_q(uiet)fatal?

#define __log_invoke(tag,fmt,...) do { \
	__log_basic(tag, __func__, __FILE__, __LINE__, fmt, ##__VA_ARGS__); \
} while (0)

#define log_fatal(fmt,...) do { \
	__log_invoke("F", fmt, ##__VA_ARGS__); \
	__log_backtrace(1); \
	abort(); \
} while (0)


#define CHECK(assertion,fmt,...) do { \
	if (!(assertion)) { \
		log_fatal(fmt, ##__VA_ARGS__); \
	} \
} while (0)

#define log_plain(fmt,...) __log_invoke(" ", fmt, ##__VA_ARGS__)

#ifndef NO_LOG_INFO
#define log_info(fmt,...) __log_invoke("i", fmt, ##__VA_ARGS__)
#else
#define log_info(...) (void)(0)
#endif

#define log_error(fmt,...) __log_invoke("E", fmt, ##__VA_ARGS__)

#ifndef LOG_VERBOSITY
#define LOG_VERBOSITY 0
#endif

#define IF_LOG_VERBOSE(level) if (LOG_VERBOSITY >= level)

// TODO: other header
#define log_verbose(level,fmt,...) do { \
	if (LOG_VERBOSITY >= level) { \
		__log_invoke("v", fmt, ##__VA_ARGS__); \
	} \
} while (0)

#include "util/likeliness.h"

// A bit like assert(x), but evaluated even with NDEBUG.
// TODO: Maybe a better name? Something like "MUST_BE_TRUE" might be better.
#ifdef NDEBUG
#define ASSERT(x) do { \
	if (unlikely(!(x))) { \
		log_fatal("ASSERT(%s) failed", #x); \
	} \
} while (0)
#else
#define ASSERT(x) assert(x)
#endif

#endif
