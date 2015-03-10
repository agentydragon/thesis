#include "experiments/performance/flags.h"

#include <assert.h>
#include <argp.h>
#include <stdlib.h>
#include <string.h>

#include "dict/array.h"
#include "dict/btree.h"
#include "dict/cobt.h"
#include "dict/dict.h"
#include "dict/splay.h"
#include "log/log.h"
#include "util/human.h"

#define COUNTOF(x) (sizeof(x) / sizeof(*x))

static const dict_api* ALL_APIS[] = {
	&dict_array, &dict_btree, &dict_cobt, &dict_splay, NULL
};

static int parse_option(int key, char *arg, struct argp_state *state) {
	(void) arg; (void) state;
	switch (key) {
	case 'm':
		FLAGS.maximum = parse_human_i(arg);
		log_info("max=%" PRIu64, FLAGS.maximum);
		break;
	case 'b':
		FLAGS.base = parse_human_d(arg);
		break;
	case 'a': {
		const char comma[] = ",";
		char* token = strtok(arg, comma);
		uint64_t i;
		for (i = 0; token != NULL; i++, token = strtok(NULL, comma)) {
			const dict_api* found = NULL;
			for (const dict_api** api = &ALL_APIS[0]; *api; ++api) {
				if (strcmp((*api)->name, token) == 0) {
					found = *api;
					break;
				}
			}
			if (found == NULL) {
				log_fatal("no such dict api: %s", token);
			}
			assert(i < COUNTOF(FLAGS.measured_apis));
			FLAGS.measured_apis[i] = found;
		}
		FLAGS.measured_apis[i] = NULL;
		break;
	}
	case ARGP_KEY_ARG:
		log_fatal("unexpected argument: %s", arg);
		break;
	}
	return 0;
}

static void set_defaults() {
	FLAGS.maximum = 1024 * 1024 * 1024;
	FLAGS.base = 1.2;
	FLAGS.measured_apis[0] = &dict_btree;
	FLAGS.measured_apis[1] = &dict_cobt;
	FLAGS.measured_apis[2] = &dict_splay;
	FLAGS.measured_apis[3] = NULL;
}

void parse_flags(int argc, char** argv) {
	set_defaults();

	struct argp_option options[] = {
		{
			.name = 0, .key = 'm', .arg = "MAX", .flags = 0,
			.doc = "Store MAX elements to store at most", .group = 0
		}, {
			.name = 0, .key = 'b', .arg = "BASE", .flags = 0,
			.doc = "Multiply by BASE at each iteration", .group = 0
		}, {
			.name = 0, .key = 'a', .arg = "APIs", .flags = 0,
			.doc = "Measured APIs", .group = 0
		}, { 0 }
	};
	struct argp argp = {
		.options = options, .parser = parse_option,
		.args_doc = NULL, .doc = NULL, .children = NULL,
		.help_filter = NULL, .argp_domain = NULL
	};
	assert(!argp_parse(&argp, argc, argv, 0, 0, 0));
}
