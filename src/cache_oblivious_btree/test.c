#include "test.h"
#include "cache_oblivious_btree.h"
#include "../log/log.h"
#include "../math/math.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNDEF 0xBADF00D
#define NIL 0xDEADDEADDEADDEAD

// Used for debugging.
static void __attribute__((unused)) dump_cob(struct cob cob) {
	log_info("veb height=%" PRIu64, cob.veb_height);
	char buffer[256];
	uint64_t offset = 0;
	for (uint64_t i = 0; i < cob.file.capacity; i++) {
		if (i % 4 == 0 && i != 0) {
			offset += sprintf(buffer + offset, "\n");
		}
		if (cob.file.occupied[i]) {
			offset += sprintf(buffer + offset, "%3" PRIu64 " ",
					cob.file.contents[i]);
		} else {
			offset += sprintf(buffer + offset, "--- ");
		}
	}
	log_info("contents={%s}", buffer);
	offset = 0;
	for (uint64_t i = 0; i < (1ULL << cob.veb_height) - 1; i++) {
		offset += sprintf(buffer + offset, "%3" PRIu64 " ",
				cob.veb_minima[i]);
	}
	log_info("veb_minima=%s", buffer);
}

#define COUNTOF(x) (sizeof(x) / sizeof(*(x)))

#define make_file(_file,...) do { \
	const uint64_t _values[] = { __VA_ARGS__ }; \
	const uint64_t _count = COUNTOF(_values); \
	assert(is_pow2(_count)); \
	struct ordered_file* file = (_file); \
	file->occupied = malloc(sizeof(bool) * _count); \
	file->contents = malloc(sizeof(uint64_t) * _count); \
	file->capacity = _count; \
	for (uint64_t i = 0; i < _count; i++) { \
		if (_values[i] == NIL) { \
			file->occupied[i] = false; \
			file->contents[i] = UNDEF; \
		} else { \
			file->occupied[i] = true; \
			file->contents[i] = _values[i]; \
		} \
	} \
} while (0)

void destroy_file(struct ordered_file file) {
	free(file.occupied);
	free(file.contents);
}

#define assert_previous_key(cob,key,previous_key) do { \
	bool found; \
	uint64_t found_key; \
	cob_previous_key(cob, key, &found, &found_key); \
	if (previous_key != NIL) { \
		if (!found) { \
			log_fatal("no previous key for %" PRIu64 ", expected " \
					"%" PRIu64, key, previous_key); \
		} \
		assert(previous_key == found_key); \
	} else { \
		assert(!found); \
	} \
} while (0)

void check_key_sequence(struct cob cob,
		const uint64_t* sequence, uint64_t count) {
	// Check sequence inside.
	for (uint64_t i = 0; i < count; i++) {
		if (sequence[i] == NIL) continue;

		for (uint64_t j = i + 1; j < count; j++) {
			if (sequence[j] == NIL) {
				continue;
			} else {
				assert_previous_key(&cob, sequence[j], sequence[i]);
				break;
			}

			/*
			cob_next_key(&cob, sequence[i], &found_found,
					&found);
			assert(found_found && found == sequence[j]);
			*/
		}
	}

	// Check that previous(first) doesn't exist and next(first-1) == first.
	for (uint64_t i = 0; i < count; i++) {
		if (sequence[i] != NIL) {
			assert_previous_key(&cob, sequence[i], NIL);

			/*
			cob_next_key(&cob, sequence[i] - 1, &found_found,
						&found);
			assert(found_found && found == sequence[i]);
			*/
			break;
		}
	}

	// Check that next(last) doesn't exist and previous(last+1) == last.
	for (uint64_t i = 0; i < count; i++) {
		uint64_t j = count - 1 - i;
		if (sequence[j] != NIL) {
			/*
			cob_next_key(&cob, sequence[j], &found_found, &found);
			assert(!found_found);
			*/

			assert_previous_key(&cob, sequence[j] + 1, sequence[j]);
			break;
		}
	}
}

#define assert_content(cob,...) do { \
	const uint64_t _expected[] = { __VA_ARGS__ }; \
	const uint64_t _count = sizeof(_expected) / sizeof(*_expected); \
	for (uint64_t i = 0; i < _count; i++) { \
		if (_expected[i] == NIL) { \
			assert(!cob.file.occupied[i]); \
		} else { \
			assert(cob.file.occupied[i]); \
			assert(cob.file.contents[i] == _expected[i]); \
		} \
	} \
	check_key_sequence(cob, _expected, _count); \
} while (0)

static void test_simple_delete() {
	// File block size: 4
	uint64_t veb_minima[] = {
		10, /* [0..7] */
		10, /* [0..3] */
		40  /* [4..7] */
	};
	struct cob cob = {
		.veb_height = 2,
		.veb_minima = veb_minima
	};
	make_file(&cob.file,
		NIL, 10, 20, 30,
		40, NIL, 50, NIL);

	cob_delete(&cob, 10);

	assert_content(cob,
		NIL, 20, NIL, 30,
		40, NIL, 50, NIL);
	assert(cob.veb_minima[0] == 20);
	assert(cob.veb_minima[1] == 20);
	assert(cob.veb_minima[2] == 40);
	destroy_file(cob.file);
}

static void test_complex_delete() {
	// van Emde Boas order: (will have 8 leaves)
	//          0
	//     1          2
	//   3   6     9     12
	//  4 5 7 8  10 11 13  14
	uint64_t veb_minima[] = {
		10, 10, 50, 10, 10, 20, 30, 30,
		40, 50, 50, 80, 100, 100, 131
	};
	struct cob cob = {
		.veb_height = 4,
		.veb_minima = veb_minima
	};
	make_file(&cob.file,
		NIL, NIL, 10, NIL,
		NIL, 20, NIL, NIL,
		NIL, NIL, NIL, 30,
		40, NIL, NIL, NIL,

		50, 60, 70, NIL,
		80, 90, NIL, NIL,
		100, 110, 120, 130,
		131, 132, 140, 150);

	cob_delete(&cob, 40);
	assert_content(cob,
			NIL, 10, NIL, 20,
			NIL, 30, NIL, 50,
			NIL, 60, NIL, 70,
			NIL, 80, NIL, 90,

			NIL, 100, NIL, 110,
			NIL, 120, NIL, 130,
			NIL, 131, NIL, 132,
			NIL, 140, NIL, 150);

	const uint64_t expected_veb_minima[] = {
		10, 10, 100, 10, 10, 30, 60, 60,
		80, 100, 100, 120, 131, 131, 140
	};
	assert(memcmp(expected_veb_minima, cob.veb_minima, sizeof(expected_veb_minima)) == 0);
	destroy_file(cob.file);
}

static void test_simple_insert() {
	// van Emde Boas order: (will have 4 leaves)
	//     0
	//  1    4
	// 2 3  5 6
	uint64_t veb_minima[] = { 100, 100, 100, 500, 700, 700, 800 };
	struct cob cob = { .veb_height = 3, .veb_minima = veb_minima };
	make_file(&cob.file,
		100, 200, 300, 400,
		500, 600, NIL, NIL,
		NIL, NIL, NIL, 700,
		NIL, NIL, 800, 900);

	cob_insert(&cob, 542);
	assert_content(cob,
		100, 200, 300, 400,
		500, 542, 600, NIL,
		NIL, NIL, NIL, 700,
		NIL, NIL, 800, 900);
	dump_cob(cob);
	destroy_file(cob.file);
}

static void test_insert_new_minimum_with_overflow() {
	// van Emde Boas order: (will have 4 leaves)
	//     0
	//  1    4
	// 2 3  5 6
	uint64_t veb_minima[] = { 100, 100, 100, 500, 700, 700, 800 };
	struct cob cob = { .veb_height = 3, .veb_minima = veb_minima };
	make_file(&cob.file,
		100, 200, 300, 400,
		500, 600, NIL, NIL,
		NIL, NIL, NIL, 700,
		NIL, NIL, 800, 900);

	cob_insert(&cob, 42);
	assert_content(cob,
		42, 100, 200, NIL,
		300, NIL, 400, NIL,
		500, NIL, 600, 700,
		NIL, 800, NIL, 900);
	const uint64_t expected_veb_minima[] = {
		42, 42, 42, 300, 500, 500, 800
	};
	assert(memcmp(expected_veb_minima, cob.veb_minima, sizeof(expected_veb_minima)) == 0);
	destroy_file(cob.file);
}

void test_cache_oblivious_btree() {
	test_simple_delete();
	test_complex_delete();
	test_simple_insert();
	// TODO: insert smallest element
	// TODO: overflow in insert
	test_insert_new_minimum_with_overflow();
}
