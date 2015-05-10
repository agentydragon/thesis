#include "experiments/vcr/playback.h"

#include <stdlib.h>
#include <string.h>

#include "log/log.h"
#include "rand/rand.h"
#include "measurement/stopwatch.h"
#include "util/consume.h"
#include "util/fnv.h"

struct {
	operation_type operation;
	const char* keyword;
} keywords[] = {
	{ .operation = FIND, .keyword = "find" },
	{ .operation = INSERT, .keyword = "insert" },
	{ .operation = DELETE, .keyword = "delete" },
};

static void insert_operation(recording* this, recorded_operation operation) {
	if (this->capacity == this->length) {
		if (this->capacity == 0) {
			this->capacity = 4;
		} else {
			this->capacity *= 2;
		}
		this->operations = realloc(this->operations,
				this->capacity * sizeof(recorded_operation));
	}
	this->operations[this->length++] = operation;
}

static uint8_t parse_hex(char c) {
	if (c >= '0' && c <= '9') {
		return (c - '0');
	} else if (c >= 'a' && c <= 'f') {
		return (c - 'a') + 10;
	} else {
		log_fatal("unparseable hex char %c", c);
	}
}

static recorded_operation parse_line(const char* line) {
	bool found = false;
	recorded_operation operation;
	const char* key;
	for (uint8_t i = 0; i < sizeof(keywords) / sizeof(*keywords); ++i) {
		if (memcmp(line, keywords[i].keyword,
					strlen(keywords[i].keyword) - 1) == 0) {
			found = true;
			operation.operation = keywords[i].operation;
			key = line + strlen(keywords[i].keyword) + 1;
			break;
		}
	}
	CHECK(found, "cannot parse line: %s", line);

	// Try to parse the key, pretending it's a 64-bit integer.
	// (It probably won't actually be, through.)
	operation.key = 0;

	uint64_t nibbles = 0;
	for (char const * octet_ptr = key; *octet_ptr && *octet_ptr != '\n';
			++octet_ptr) {
		++nibbles;
	}
	if (nibbles <= 8) {
		// The key is at most 64-bit, it will fit into our data
		// structures.
		while (*key && *key != '\n') {
			operation.key <<= 4;
			operation.key |= parse_hex(*key);
			++key;
		}
	} else {
		// Fold the key via FNV-1 to fit.
		fnv1_state state = fnv1_begin();
		while (*key && *key != '\n') {
			fnv1_add(&state, *key);
			++key;
		}
		operation.key = fnv1_finish(state);
	}
	return operation;
}

recording* load_recording(const char* path) {
	recording* record = malloc(sizeof(recording));
	CHECK(record, "cannot alloc memory for recording");
	record->length = 0;
	record->capacity = 0;
	record->operations = NULL;

	FILE* fp = fopen(path, "r");
	CHECK(fp, "cannot open recording at %s", path);
	char line[1024];
	while (!feof(fp)) {
		if (fgets(line, sizeof(line), fp) == NULL) {
			// TODO: I'd really like to ignore the return value.
			break;
		}
		recorded_operation operation = parse_line(line);
		insert_operation(record, operation);
	}
	fclose(fp);

	return record;
}

void destroy_recording(recording* this) {
	free(this->operations);
	free(this);
}

struct metrics measure_recording(const dict_api* api, recording* record,
		int repetitions) {
	measurement* measurement = measurement_begin();
	stopwatch watch = stopwatch_start();
	rand_generator rand = { .state = 0 };

	for (int repeat = 0; repeat < repetitions; ++repeat) {
		dict* dict;
		dict_init(&dict, api);

		for (uint64_t i = 0; i < record->length; ++i) {
			const recorded_operation operation = record->operations[i];
			uint64_t value;

			switch (operation.operation) {
			case FIND: {
				bool found = dict_find(dict, operation.key, &value);
				consume_bool(found);
				consume64(value);
				break;
			}
			case INSERT: {
				// Note: dict_insert may fail (since
				// the recording may have been cleaned).
				value = rand_next(&rand, UINT64_MAX);
				if (!dict_insert(dict, operation.key, value)) {
					log_verbose(1, "insert failure");
				}
				break;
			}
			case DELETE: {
				// Note: dict_delete may fail (since
				// the recording may have been cleaned).
				if (!dict_delete(dict, operation.key)) {
					log_verbose(1, "delete failure");
				}
				break;
			}
			}
		}

		dict_destroy(&dict);
	}

	measurement_results* results = measurement_end(measurement);
	uint64_t time_nsec = stopwatch_read_ns(watch);
	return (struct metrics) {
		.results = results,
		.time_nsec = time_nsec
	};
}
