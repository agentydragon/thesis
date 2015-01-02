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
	log_info("capacity=%" PRIu64 " block_size=%" PRIu64,
			cob.file.parameters.capacity,
			cob.file.parameters.block_size);
	char buffer[256];
	uint64_t offset = 0;
	for (uint64_t i = 0; i < cob.file.parameters.capacity; i++) {
		if (i % cob.file.parameters.block_size == 0 && i != 0) {
			offset += sprintf(buffer + offset, "\n");
		}
		if (cob.file.occupied[i]) {
			offset += sprintf(buffer + offset, "%3" PRIu64 " ",
					cob.file.keys[i]);
		} else {
			offset += sprintf(buffer + offset, "--- ");
		}
	}
	log_info("keys={%s}", buffer);
	offset = 0;
	for (uint64_t i = 0; i < (1ULL << cob.get_veb_height()) - 1; i++) {
		offset += sprintf(buffer + offset, "%3" PRIu64 " ",
				cob.veb_minima[i]);
	}
	log_info("veb_minima=%s", buffer);
}

#define COUNTOF(x) (sizeof(x) / sizeof(*(x)))

#define make_file(_file,_block_size,...) do { \
	const uint64_t _values[] = { __VA_ARGS__ }; \
	const uint64_t _count = COUNTOF(_values); \
	assert(is_pow2(_count)); \
	struct ordered_file* file = (_file); \
	file->occupied = malloc(sizeof(bool) * _count); \
	file->keys = malloc(sizeof(uint64_t) * _count); \
	\
	file->parameters.capacity = _count; \
	file->parameters.block_size = _block_size; \
	for (uint64_t i = 0; i < _count; i++) { \
		if (_values[i] == NIL) { \
			file->occupied[i] = false; \
			file->keys[i] = UNDEF; \
		} else { \
			file->occupied[i] = true; \
			file->keys[i] = _values[i]; \
		} \
	} \
} while (0)

void destroy_file(struct ordered_file file) {
	free(file.occupied);
	free(file.keys);
}

#define assert_next_key(cob,key,_next_key) do { \
	bool found; \
	uint64_t found_key; \
	cob->next_key(key, &found, &found_key); \
	if (_next_key != NIL) { \
		if (!found) { \
			log_fatal("no next key for %" PRIu64 ", expected " \
					"%" PRIu64, key, _next_key); \
		} \
		assert(_next_key == found_key); \
	} else { \
		assert(!found); \
	} \
} while (0)

#define assert_previous_key(cob,key,_previous_key) do { \
	bool found; \
	uint64_t found_key; \
	cob->previous_key(key, &found, &found_key); \
	if (_previous_key != NIL) { \
		CHECK(found, \
				"no previous key for %" PRIu64 ", expected " \
				"%" PRIu64, key, _previous_key); \
		CHECK(_previous_key == found_key, \
				"previous key to %" PRIu64 " should have been " \
				"%" PRIu64 ", but it is actually %" PRIu64, \
				key, _previous_key, found_key); \
	} else { \
		assert(!found); \
	} \
} while (0)

#define insert_items(cob,...) do { \
	const uint64_t items[] = { __VA_ARGS__ }; \
	for (uint64_t i = 0; i < sizeof(items) / sizeof(*items); i++) { \
		cob.insert(cob, items[i]); \
	} \
} while (0)

#define delete_items(cob,...) do { \
	const uint64_t items[] = { __VA_ARGS__ }; \
	for (uint64_t i = 0; i < sizeof(items) / sizeof(*items); i++) { \
		cob.remove(cob, items[i]); \
	} \
} while (0)

void check_key_sequence(const struct cob* cob,
		const uint64_t* sequence, uint64_t count) {
	// Check sequence inside.
	for (uint64_t i = 0; i < count; i++) {
		if (sequence[i] == NIL) continue;

		bool found;
		cob->has_key(sequence[i], &found);
		assert(found);

		for (uint64_t j = i + 1; j < count; j++) {
			if (sequence[j] == NIL) {
				continue;
			} else {
				assert_previous_key(cob, sequence[j], sequence[i]);
				assert_next_key(cob, sequence[i], sequence[j]);
				break;
			}
		}
	}

	// Check that previous(first) doesn't exist and next(first-1) == first.
	for (uint64_t i = 0; i < count; i++) {
		if (sequence[i] != NIL) {
			assert_previous_key(cob, sequence[i], NIL);
			assert_next_key(cob, sequence[i] - 1, sequence[i]);
			break;
		}
	}

	// Check that next(last) doesn't exist and previous(last+1) == last.
	for (uint64_t i = 0; i < count; i++) {
		uint64_t j = count - 1 - i;
		if (sequence[j] != NIL) {
			assert_previous_key(cob, sequence[j] + 1, sequence[j]);
			assert_next_key(cob, sequence[j], NIL);
			break;
		}
	}
}

#define assert_content(__cob,...) do { \
	const struct cob* _cob = __cob; \
	const uint64_t _expected[] = { __VA_ARGS__ }; \
	const uint64_t _count = sizeof(_expected) / sizeof(*_expected); \
	for (uint64_t i = 0; i < _count; i++) { \
		if (_expected[i] == NIL) { \
			assert(!_cob->file.occupied[i]); \
		} else { \
			assert(_cob->file.occupied[i]); \
			assert(_cob->file.keys[i] == _expected[i]); \
		} \
	} \
	check_key_sequence(_cob, _expected, _count); \
} while (0)

#define assert_content_low_detail(__cob,...) do { \
	const struct cob* _cob = __cob; \
	const uint64_t _expected[] = { __VA_ARGS__ }; \
	const uint64_t _count = sizeof(_expected) / sizeof(*_expected); \
	check_key_sequence(_cob, _expected, _count); \
} while (0)

static void test_simple_delete() {
	// File block size: 4
	uint64_t veb_minima[] = {
		10, /* [0..7] */
		10, /* [0..3] */
		40  /* [4..7] */
	};
	struct cob cob;
	cob.veb_minima = veb_minima;
	make_file(&cob.file, 4,
		NIL, 10, 20, 30,
		40, NIL, 50, NIL);

	cob.remove(10);

	assert_content(&cob,
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
		.veb_minima = veb_minima
	};
	make_file(&cob.file, 4,
		NIL, NIL, 10, NIL,
		NIL, 20, NIL, NIL,
		NIL, NIL, NIL, 30,
		40, NIL, NIL, NIL,

		50, 60, 70, NIL,
		80, 90, NIL, NIL,
		100, 110, 120, 130,
		131, 132, 140, 150);

	cob.remove(40);
	assert_content(&cob,
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
	struct cob cob = { .veb_minima = veb_minima };
	make_file(&cob.file, 4,
		100, 200, 300, 400,
		500, 600, NIL, NIL,
		NIL, NIL, NIL, 700,
		NIL, NIL, 800, 900);

	cob.insert(542);
	assert_content(&cob,
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
	struct cob cob = { .veb_minima = veb_minima };
	make_file(&cob.file, 4,
		100, 200, 300, 400,
		500, 600, NIL, NIL,
		NIL, NIL, NIL, 700,
		NIL, NIL, 800, 900);

	cob.insert(42);
	assert_content(&cob,
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

static void test_comprehensive_resizing() {
	uint64_t* veb_minima = malloc(sizeof(uint64_t));
	veb_minima[0] = COB_INFINITY;

	struct cob cob = { .veb_minima = veb_minima };
	make_file(&cob.file, 4, NIL, NIL, NIL, NIL);

	insert_items(&cob,
		200, 300, 400, 600, 800, 100, 150, 250, 450, 900, 910, 920,
		120, 130, 140, 170, 190, 210, 230, 270, 310, 810, 820, 990,
		135, 137, 148, 149, 660, 670, 666, 142, 147, 550, 560, 775,
		143, 157, 168, 152, 915, 965, 963, 998, 916, 971, 561, 567);

	// Negative assertion against _found.
	bool _found;
	cob.has_key(&cob, 667, &_found);
	assert(!_found);

	assert_content_low_detail(&cob,
		100, 120, 130, 135, 137, 140, 142, 143, 147, 148, 149, 150,
		152, 157, 168, 170, 190, 200, 210, 230, 250, 270, 300, 310,
		400, 450, 550, 560, 561, 567, 600, 660, 666, 670, 775, 800,
		810, 820, 900, 910, 915, 916, 920, 963, 965, 971, 990, 998);

	delete_items(&cob,
		149, 660, 550, 400, 157, 998, 916, 920, 450, 100, 915, 971);
	assert_content_low_detail(&cob,
		120, 130, 135, 137, 140, 142, 143, 147, 148, 150, 152, 168,
		170, 190, 200, 210, 230, 250, 270, 300, 310, 560, 561, 567,
		600, 666, 670, 775, 800, 810, 820, 900, 910, 963, 965, 990);

	delete_items(&cob,
		137, 147, 168, 120, 130, 135, 300, 910, 810, 965, 567, 600);
	assert_content_low_detail(&cob,
		140, 142, 143, 148, 150, 152, 170, 190, 200, 210, 230, 250,
		270, 310, 560, 561, 666, 670, 775, 800, 820, 900, 963, 990);

	delete_items(&cob,
		270, 310, 560, 561, 666, 152, 170, 190, 200, 900, 963, 990);
	assert_content_low_detail(&cob,
		140, 142, 143, 148, 150, 210, 230, 250, 670, 775, 800, 820);

	delete_items(&cob, 143, 148, 210, 230, 250, 670, 820, 150);
	assert_content_low_detail(&cob, 140, 142, 775, 800);

	delete_items(&cob, 142, 775, 800, 140);
	assert_content_low_detail(&cob);

	// Check minimum calculation in empty tree.
	assert(cob.veb_minima[0] == COB_INFINITY);

	destroy_file(cob.file);
	free(cob.veb_minima);
}

void test_cache_oblivious_btree() {
	test_simple_delete();
	test_complex_delete();
	test_simple_insert();
	test_insert_new_minimum_with_overflow();

	test_comprehensive_resizing();
}
