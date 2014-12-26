#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

void __log_basic(const char* tag, const char* format, ...);
void log_fatal(const char* format, ...);

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

#define CHECK(x) do { \
	if (!x) { \
		log_error("check failed: " #x); \
		abort(); \
	} \
} while (0)

#endif
