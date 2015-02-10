#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

void __log_basic(const char* tag, const char* format, ...);
void log_fatal(const char* format, ...) __attribute__((noreturn));

#define CHECK(assertion,message,...) do { \
	if (!(assertion)) { \
		log_fatal(message, ##__VA_ARGS__); \
	} \
} while (0)

#define log_plain(fmt,...) do { \
	__log_basic("     ", fmt, ##__VA_ARGS__); \
} while (0)

#ifndef NO_LOG_INFO
#define log_info(fmt,...) do { \
	__log_basic(" INFO", fmt, ##__VA_ARGS__); \
} while (0)
#else
#define log_info(...) (void)(0)
#endif

#define log_error(fmt,...) do { \
	__log_basic("ERROR", fmt, ##__VA_ARGS__); \
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

#endif
