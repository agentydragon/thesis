#include "log.h"

#define _GNU_SOURCE  /* vasprintf */
#include <stdio.h>

#include <assert.h>
#include <stdlib.h>

#include <stdarg.h>
#include <execinfo.h>

void __log_v(const char* tag, const char* format, va_list args) {
	char* message;
	assert(vasprintf(&message, format, args) >= 0);
	fprintf(stderr, "%s: %s\n", tag, message);
	free(message);
}

void __log_basic(const char* tag, const char* format, ...) {
	va_list args;
	va_start(args, format);
	__log_v(tag, format, args);
	va_end(args);
}

// TODO: Extract line numbers
static void log_backtrace(int stripped_levels) {
	void *frames[100];
	int size = backtrace(frames, 100);
	char **strings = backtrace_symbols(frames, size);

	// Strip one more level (us).
	for (int i = stripped_levels + 1; i < size; i++) {
		log_plain("[%3d] %s", i - stripped_levels, strings[i]);
	}
	free(strings);
}

void log_fatal(const char* format, ...) {
	va_list args;
	va_start(args, format);
	__log_v(" FATAL", format, args);
	va_end(args);

	log_backtrace(1);

	abort();
}
