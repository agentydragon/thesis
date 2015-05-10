#include "experiments/vcr/flags.h"

#include <string.h>

// NOTE: See //experiments/performance/flags.c for important note
// about <argp.h> include order.
#include <argp.h>

#include "dict/array.h"
#include "dict/btree.h"
#include "dict/cobt.h"
#include "dict/dict.h"
#include "dict/kforest.h"
#include "dict/ksplay.h"
#include "dict/register.h"
#include "dict/splay.h"
#include "log/log.h"
#include "util/count_of.h"

static int parse_option(int key, char *arg, struct argp_state *state) {
	(void) arg; (void) state;
	switch (key) {
	case 'p':
		// TODO: Does someone need to free it when we're done?
		FLAGS.recording_path = strdup(arg);
		break;
	case 'a': {
		dict_api_list_parse(arg, FLAGS.measured_apis,
				COUNT_OF(FLAGS.measured_apis));
		break;
	}
	case 'r': {
		FLAGS.repetitions = atoi(arg);
		ASSERT(FLAGS.repetitions > 0);
		break;
	}
	case ARGP_KEY_ARG:
		log_fatal("unexpected argument: %s", arg);
		break;
	}
	return 0;
}

static void set_defaults(void) {
	// TODO: Unify this with performance/flags.c somewhere ("dict/all.h"?)
	FLAGS.measured_apis[0] = &dict_btree;
	FLAGS.measured_apis[1] = &dict_cobt;
	FLAGS.measured_apis[2] = &dict_splay;
	FLAGS.measured_apis[3] = &dict_kforest;
	FLAGS.measured_apis[4] = &dict_ksplay;
	FLAGS.measured_apis[5] = NULL;

	FLAGS.repetitions = 1;

	FLAGS.recording_path = NULL;
}

void parse_flags(int argc, char** argv) {
	set_defaults();

	struct argp_option options[] = {
		{
			.name = 0, .key = 'p', .arg = "PATH", .flags = 0,
			.doc = "Path to text-formatted recording", .group = 0
		}, {
			.name = 0, .key = 'r', .arg = "REPETITIONS", .flags = 0,
			.doc = "Number of repetitions, 1 by default", .group = 0
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
	CHECK(!argp_parse(&argp, argc, argv, 0, 0, 0), "argp_parse failed");

	if (FLAGS.recording_path == NULL) {
		log_fatal("You must set a path using -p.");
	}

	// TODO: Make it possible to override the default dicts.
}
