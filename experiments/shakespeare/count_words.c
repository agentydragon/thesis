#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#include "../../log/log.h"
#include "../../stopwatch/stopwatch.h"
#include "../../hash/hash.h"
#include "../../hash_hashtable/hash_hashtable.h"

hash* word_count;

static uint64_t hash_word(const char* word) {
	uint64_t hash = 14695981039346656037ULL;
	for (const char* p = word; *p; p++) {
		hash = (hash * 1099511628211LL) ^ (*p);
	}
	return hash;

	/*
	// For debugging purposes
	uint64_t hash = 0;
	for (int i = 0; word[i]; i++) {
		((uint8_t*)&hash)[i % 8] ^= word[i];
	}
	return hash;
	*/
}

static char* str_normalize(char* s) {
	char* normalized = strdup(s);
	for (char* p = normalized; *p; p++) {
		*p = tolower(*p);
	}
	return normalized;
}

static void add_word(char* word) {
	char* normalized = str_normalize(word);
	uint64_t key = hash_word(normalized);

	bool found;
	uint64_t value;
	assert(!hash_find(word_count, key, &value, &found));
	if (found) {
		// printf("[%s]: %" PRIu64 " -> %" PRIu64 "\n", normalized, key, value + 1);
		assert(!hash_delete(word_count, key));
		assert(!hash_insert(word_count, key, value + 1));
	} else {
		// printf("[%s]: %" PRIu64 " = 1 (not found yet)\n", normalized, key);
		assert(!hash_insert(word_count, key, 1));
	}

	free(normalized);
}

static void process_line(char* line) {
	const char* delimiters = " ,.?!*<>()[]:;'\n";
	char* ptr = strtok(line, delimiters);
	while (ptr) {
		add_word(ptr);
		ptr = strtok(NULL, delimiters);
	}
}

static void report_count(const char* word) {
	uint64_t key = hash_word(word);

	bool found;
	uint64_t value;
	assert(!hash_find(word_count, key, &value, &found));
	if (found) {
		printf("%s: found %" PRIu64 " times\n", word, value);
	} else {
		printf("%s: not found\n", word);
	}
}

#define PATH "shakespeare.txt"

int main(int argc, char** argv) {
	(void) argc; (void) argv;

	assert(!hash_init(&word_count, &hash_hashtable, NULL));

	FILE* f = fopen(PATH, "r");
	if (!f) {
		log_fatal("Cannot open " PATH);
	}

	char line[65535];

	printf("Counting words...\n");

	stopwatch watch = stopwatch_start();
	stopwatch watch_1000 = stopwatch_start();

	uint64_t lineno = 0;
	while (!feof(f)) {
		fgets(line, sizeof(line), f);
		process_line(line);
		lineno++;

		printf("line: %s\n", line);

		if (lineno % 1000 == 0) {
			printf("line %" PRIu64 ", last 1000 lines took %" PRIu64 " ms\n", lineno, stopwatch_read_ms(watch_1000));
			watch_1000 = stopwatch_start();
		}
	}

	fclose(f);

	uint64_t s, ms;
	stopwatch_read_s_ms(watch, &s, &ms);
	printf("Finished in %" PRIu64 ".%03" PRIu64 " s .\n", s, ms);

	report_count("the");
	report_count("obsequious");
	report_count("hello");
	report_count("world");
}
