#include "experiments/performance/flags.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// NOTE: <argp.h> MUST be included after <string.h>.
// Including <string.h> after <argp.h> creates infinite-loop memcpy/memmove
// and friends on lab computers for some reason.
#include <argp.h>

#include "dict/array.h"
#include "dict/btree.h"
#include "dict/cobt.h"
#include "dict/dict.h"
#include "dict/htcuckoo.h"
#include "dict/htlp.h"
#include "dict/kforest.h"
#include "dict/ksplay.h"
#include "dict/register.h"
#include "dict/rbtree.h"
#include "dict/splay.h"
#include "log/log.h"
#include "util/count_of.h"
#include "util/human.h"

static int parse_option(int key, char *arg, struct argp_state *state) {
	(void) state;
	switch (key) {
	case 'm':
		FLAGS.maximum = parse_human_i(arg);
		log_info("max=%" PRIu64, FLAGS.maximum);
		break;
	case 'b':
		FLAGS.base = parse_human_d(arg);
		break;
	case 'a': {
		dict_api_list_parse(arg, FLAGS.measured_apis,
				COUNT_OF(FLAGS.measured_apis));
		break;
	}
	case ARGP_KEY_ARG:
		log_fatal("unexpected argument: %s", arg);
		break;
	}
	return 0;
}

static dict_api const * const DEFAULT_APIS[] = {
	&dict_btree,
	&dict_cobt,
	&dict_htcuckoo,
	&dict_htlp,
	&dict_kforest,
	&dict_ksplay,
	&dict_rbtree,
	&dict_splay,
	NULL
};

static void set_defaults(void) {
	FLAGS.maximum = 1024 * 1024 * 1024;
	FLAGS.base = 1.2;

	// TODO: use dict_register_grab, and filter out unreasonably slow APIs
	// later. It would be interesting to see, for example,
	// simple sorted arrays.
	memcpy(FLAGS.measured_apis, DEFAULT_APIS, sizeof(DEFAULT_APIS));
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
	CHECK(!argp_parse(&argp, argc, argv, 0, 0, 0), "argp_parse failed");
}
