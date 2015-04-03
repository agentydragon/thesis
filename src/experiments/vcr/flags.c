#include "experiments/vcr/flags.h"

#include <string.h>

// NOTE: See //experiments/performance/flags.c for important note
// about <argp.h> include order.
#include <argp.h>

#include "dict/array.h"
#include "dict/btree.h"
#include "dict/cobt.h"
#include "dict/dict.h"
#include "dict/htable.h"
#include "dict/kforest.h"
#include "dict/ksplay.h"
#include "dict/splay.h"
#include "log/log.h"

static int parse_option(int key, char *arg, struct argp_state *state) {
	(void) arg; (void) state;
	switch (key) {
	case 'p':
		// TODO: Does someone need to free it when we're done?
		FLAGS.recording_path = strdup(arg);
		break;
	case ARGP_KEY_ARG:
		log_fatal("unexpected argument: %s", arg);
		break;
	}
	return 0;
}

static void set_defaults() {
	// TODO: Unify this with performance/flags.c somewhere ("dict/all.h"?)
	FLAGS.measured_apis[0] = &dict_btree;
	FLAGS.measured_apis[1] = &dict_cobt;
	FLAGS.measured_apis[2] = &dict_htable;
	FLAGS.measured_apis[3] = &dict_splay;
	FLAGS.measured_apis[4] = &dict_kforest;
	FLAGS.measured_apis[5] = &dict_ksplay;
	FLAGS.measured_apis[6] = NULL;

	FLAGS.recording_path = NULL;
}

void parse_flags(int argc, char** argv) {
	set_defaults();

	struct argp_option options[] = {
		{
			.name = 0, .key = 'p', .arg = "PATH", .flags = 0,
			.doc = "Path to text-formatted recording", .group = 0
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
