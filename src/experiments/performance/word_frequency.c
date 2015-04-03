#include "experiments/performance/word_frequency.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "dict/ksplay.h"
#include "ksplay/ksplay.h"
#include "log/log.h"
#include "measurement/stopwatch.h"

static char* read_file(const char* path) {
	FILE* fp = fopen(path, "r");
	CHECK(fp, "cannot open %s for reading", path);

	char* content = NULL;
	uint64_t content_size = 0, content_capacity = 0;
	char buffer[1024];
	while (!feof(fp)) {
		fgets(buffer, sizeof(buffer), fp);
		if (content_size + strlen(buffer) >= content_capacity) {
			if (content_capacity == 0) {
				content_capacity = 1024;
			}
			content_capacity *= 2;
			content = realloc(content, content_capacity);
			CHECK(content, "failed to allocate file content");
		}
		strcpy(content + content_size, buffer);
		content_size += strlen(buffer);
	}
	fclose(fp);
	return content;
}

static const char* PATH = "experiments/performance/data/shakespeare.txt";

static void normalize(char* word) {
	while (*word) {
		*word = tolower(*word);
		++word;
	}
}

// We assume that on our corpus, words will be more or less unique.
// We could probably get around this by just smartly encoding words into
// 64-bit integers.
static uint64_t hash_word(const char* word) {
	uint64_t hash = 14695981039346656037ULL;
	for (const char* p = word; *p; p++) {
		hash = (hash * 1099511628211LL) ^ (*p);
	}
	return hash;
}

static void add_word(dict* dict, char* word) {
	normalize(word);
	uint64_t key = hash_word(word);

	bool found;
	uint64_t count;
	ASSERT(!dict_find(dict, key, &count, &found));

	if (found) {
		ASSERT(!dict_delete(dict, key));
		ASSERT(!dict_insert(dict, key, count + 1));
	} else {
		ASSERT(!dict_insert(dict, key, 1));
	}
}

static void report_count(dict* dict, const char* word) {
	uint64_t key = hash_word(word);
	bool found;
	uint64_t value;
	ASSERT(!dict_find(dict, key, &value, &found));
	if (found) {
		log_info("%s: found %" PRIu64 " times", word, value);
	} else {
		log_info("%s: not found", word);
	}
}

static char* content;

void init_word_frequency() {
	content = read_file(PATH);
}

void deinit_word_frequency() {
	free(content);
}

struct metrics measure_word_frequency(const dict_api* api, uint64_t size) {
	const char* delimiters = " ,.?!*<>()[]:;'\n";
	uint64_t words = 0;
	char* content_copy = strdup(content);
	char* ptr = strtok(content_copy, delimiters);

	dict* dict;
	CHECK(!dict_init(&dict, api, NULL), "cannot init dict");

	measurement* measurement = measurement_begin();
	stopwatch watch = stopwatch_start();

	while (ptr && words < size) {
		add_word(dict, ptr);
		ptr = strtok(NULL, delimiters);
		++words;
	}

	if (!ptr) {
		log_error("ran out of words, give me a larger corpus.");
	}

	measurement_results* results = measurement_end(measurement);
	uint64_t time_nsec = stopwatch_read_ns(watch);

	IF_LOG_VERBOSE(1) {
		report_count(dict, "the");
		report_count(dict, "obsequious");
		report_count(dict, "hello");
		report_count(dict, "world");
	}

	dict_destroy(&dict);
	free(content_copy);

	return (struct metrics) {
		.results = results,
		.time_nsec = time_nsec
	};
}
