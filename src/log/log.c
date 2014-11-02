#include "log.h"

#define _GNU_SOURCE  /* vasprintf */
#include <stdio.h>

#include <assert.h>
#include <stdlib.h>

#include <stdarg.h>

void __log_v(const char* tag, const char* format, va_list args) {
	char* message;
	assert(vasprintf(&message, format, args) >= 0);
	fprintf(stderr, "%s: %s\n", tag, message);
	free(message);
}

void __log(const char* tag, const char* format, ...) {
	va_list args;
	va_start(args, format);
	__log_v(tag, format, args);
	va_end(args);
}

void log_fatal(const char* format, ...) {
	va_list args;
	va_start(args, format);
	__log_v(" FATAL", format, args);
	va_end(args);

	abort();
}
