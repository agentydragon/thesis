#include "experiments/performance/playback.h"

#include <stdlib.h>
#include <string.h>

#include "log/log.h"
#include "rand/rand.h"
#include "measurement/stopwatch.h"
#include "measurement/measurement.h"
#include "util/consume.h"

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
	while (*key && *key != '\n') {
		const char c = *key;
		operation.key <<= 4;
		if (c >= '0' && c <= '9') {
			operation.key |= (c - '0');
		} else if (c >= 'a' && c <= 'f') {
			operation.key |= (c - 'a') + 10;
		} else {
			log_fatal("unparseable key char %c in line %s",
					c, line);
		}
		++key;
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
		fgets(line, sizeof(line), fp);
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

struct metrics measure_recording(const dict_api* api, recording* record) {
	measurement* measurement = measurement_begin();
	stopwatch watch = stopwatch_start();
	rand_generator rand = { .state = 0 };

	dict* dict;
	CHECK(!dict_init(&dict, api, NULL), "cannot init dict");

	for (uint64_t i = 0; i < record->length; ++i) {
		const recorded_operation operation = record->operations[i];
		uint64_t value;

		switch (operation.operation) {
		case FIND: {
			bool found;
			ASSERT(!dict_find(dict, operation.key, &value, &found));
			consume_bool(found);
			consume64(value);
			break;
		}
		case INSERT: {
			// Note: dict_insert may fail (since the recording
			// may have been cleaned).
			value = rand_next(&rand, UINT64_MAX);
			dict_insert(dict, operation.key, value);
			break;
		}
		case DELETE: {
			// Note: dict_delete may fail (since the recording
			// may have been cleaned).
			dict_delete(dict, operation.key);
			break;
		}
		}
	}

	measurement_results* results = measurement_end(measurement);
	uint64_t time_nsec = stopwatch_read_ns(watch);

	dict_destroy(&dict);

	return (struct metrics) {
		.results = results,
		.time_nsec = time_nsec
	};
}
