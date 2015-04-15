#include "experiments/cloud/flags.h"

#include <string.h>

// NOTE: <argp.h> MUST be included after <string.h>.
// Including <string.h> after <argp.h> creates infinite-loop memcpy/memmove
// and friends on lab computers for some reason.
#include <argp.h>

#include "dict/btree.h"
#include "dict/cobt.h"
#include "dict/kforest.h"
#include "dict/ksplay.h"
#include "dict/register.h"
#include "dict/splay.h"
#include "log/log.h"
#include "util/count_of.h"

#define FLAG_MIN 0x10000
#define FLAG_MIN_YEAR (FLAG_MIN + 1)
#define FLAG_MAX_YEAR (FLAG_MIN + 2)
#define FLAG_SCATTER (FLAG_MIN + 3)
#define FLAG_DUMP_AVERAGES (FLAG_MIN + 4)

static int parse_option(int key, char* arg, struct argp_state* state) {
	(void) state;
	switch (key) {
	case 'a': {
		dict_api_list_parse(arg, FLAGS.measured_apis,
				COUNT_OF(FLAGS.measured_apis));
		break;
	}
	case 't':
		FLAGS.lat_step = strtoll(arg, NULL, 10);
		CHECK(FLAGS.lat_step > 0, "LAT_STEP must be positive.");
		break;
	case 'n':
		FLAGS.lon_step = strtoll(arg, NULL, 10);
		CHECK(FLAGS.lon_step > 0, "LON_STEP must be positive.");
		break;
	case 's':
		FLAGS.sample_count = strtoll(arg, NULL, 10);
		break;
	case FLAG_MIN_YEAR:
		FLAGS.min_year = atoi(arg);
		break;
	case FLAG_MAX_YEAR:
		FLAGS.max_year = atoi(arg);
		break;
	case 'c':
		FLAGS.close_point_count = strtoll(arg, NULL, 10);
		break;
	case FLAG_SCATTER:
		if (strcmp(arg, "GRID") == 0) {
			FLAGS.scatter = SCATTER_GRID;
		} else if (strcmp(arg, "RANDOM") == 0) {
			FLAGS.scatter = SCATTER_RANDOM;
		} else {
			log_fatal("Bad scatter mode: %s", arg);
		}
		break;
	case FLAG_DUMP_AVERAGES:
		FLAGS.dump_averages = true;
		break;
	case ARGP_KEY_ARG:
		log_fatal("unexpected argument: %s", arg);
		break;
	}
	return 0;
}

static void set_defaults(void) {
	FLAGS.measured_apis[0] = &dict_cobt;
	FLAGS.measured_apis[1] = &dict_splay;
	FLAGS.measured_apis[2] = &dict_ksplay;
	FLAGS.measured_apis[3] = NULL;

	FLAGS.dump_averages = false;

	FLAGS.scatter = SCATTER_GRID;
	FLAGS.lat_step = 100;
	FLAGS.lon_step = 100;
	FLAGS.sample_count = 10000;

	FLAGS.min_year = 1997;
	FLAGS.max_year = 2009;

	FLAGS.close_point_count = 1000;
}

void parse_flags(int argc, char** argv) {
	set_defaults();

	struct argp_option options[] = {
		{
			.name = "apis", .key = 'a', .arg = "APIs", .flags = 0,
			.doc = "Measured APIs", .group = 0
		},
		{
			.name = "close_count", .key = 'c', .arg = "CLOSE_COUNT",
			.flags = 0, .doc = "Pred/succ queries per sample",
			.group = 0
		},
		{
			.name = "scatter", .key = FLAG_SCATTER,
			.arg = "(GRID|RANDOM)",
			.doc = "Scatter mode (regular grid or random)",
			.group = 0
		},
	        {
			.name = "lat_step", .key = 't', .arg = "LAT_STEP",
			.flags = 0, .doc = "Latitude step in 1/100's of degree",
			.group = 1
		},
	        {
			.name = "lon_step", .key = 'n', .arg = "LON_STEP",
			.flags = 0,
			.doc = "Longitude step in 1/100's of degree", .group = 1
		},
		{
			.name = "samples", .key = 's', .arg = "N",
			.flags = 0, .doc = "Number of random samples to take.",
			.group = 0
		},
		{
			.name = "min_year", .key = FLAG_MIN_YEAR,
			.arg = "MIN_YEAR", .flags = 0,
			.doc = "Minimum data year (1997..2009)", .group = 2
		},
		{
			.name = "max_year", .key = FLAG_MAX_YEAR,
			.arg = "MAX_YEAR", .flags = 0,
			.doc = "Maximum data year (1997..2009); inclusive",
			.group = 2
		},
		{
			.name = "dump_averages", .key = FLAG_DUMP_AVERAGES,
			.arg = NULL, .flags = 0,
			.doc = "If passed, averages and station positions "
					"will be dumped.", .group = 0
		},
		{ 0 }
	};
	struct argp argp = {
		.options = options, .parser = parse_option,
		.args_doc = NULL, .doc = NULL, .children = NULL,
		.help_filter = NULL, .argp_domain = NULL
	};
	CHECK(!argp_parse(&argp, argc, argv, 0, 0, 0), "argp_parse failed");

	if (FLAGS.min_year < 1997 || FLAGS.min_year > 2009) {
		log_fatal("invalid minimum year, expecting 1997..2009");
	}
	if (FLAGS.max_year < 1997 || FLAGS.max_year > 2009) {
		log_fatal("invalid maximum year, expecting 1997..2009");
	}
	if (FLAGS.min_year > FLAGS.max_year) {
		log_fatal("invalid year range, min > max");
	}
}
