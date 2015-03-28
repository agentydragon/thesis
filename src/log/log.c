#include "log/log.h"

/* Needs _GNU_SOURCE for vasprintf */
#include <stdio.h>

#include <assert.h>
#include <execinfo.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

static void format_time(char* buffer, uint64_t capacity) {
	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);
	ASSERT(tmp);
	ASSERT(strftime(buffer, capacity, "%F %T", tmp) != 0);
}

static void __log_v(const char* tag, const char* func, const char* file,
		int line, const char* format, va_list args) {
	char* message;
	ASSERT(vasprintf(&message, format, args) >= 0);

	char time[20];
	format_time(time, sizeof(time));

	char* line_spec;
	ASSERT(asprintf(&line_spec, "%s:%-4d", file, line) >= 0);

	fprintf(stderr, "%s %s %30s %s: %s\n", tag, time, line_spec, func,
			message);
	free(message);
}

void __log_basic(const char* tag, const char* func, const char* file,
		int line, const char* format, ...) {
	va_list args;
	va_start(args, format);
	__log_v(tag, func, file, line, format, args);
	va_end(args);
}

// TODO: Extract line numbers
void __log_backtrace(int stripped_levels) {
	void *frames[100];
	int size = backtrace(frames, 100);
	char **strings = backtrace_symbols(frames, size);

	// Strip one more level (us).
	for (int i = stripped_levels + 1; i < size; i++) {
		log_plain("[%3d] %s", i - stripped_levels, strings[i]);
	}
	free(strings);
}
