#include "measurement/measurement.h"

#include <asm/unistd.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <jansson.h>
#include <linux/perf_event.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "log/log.h"
#include "util/count_of.h"

typedef struct {
	const char* name;
	int perf_type;
	int perf_config;
} counter_spec;

static counter_spec counters[] = {
	{"cache_references", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_REFERENCES},
	{"cache_misses", PERF_TYPE_HARDWARE, PERF_COUNT_HW_CACHE_MISSES},
	{"branches", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_INSTRUCTIONS},
	{"branch_mispredicts", PERF_TYPE_HARDWARE, PERF_COUNT_HW_BRANCH_MISSES},
};

struct measurement {
	int* fds;
};

struct measurement_results {
	uint64_t* counters;
};

// TODO: PERF_COUNT_SW_ALIGNMENT_FAULTS
// TODO: use perf_event groups later?

static int perf_event_open(struct perf_event_attr *hw_event,
		pid_t pid, int cpu, int group_fd, unsigned long flags) {
	return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

static int event_fd_open(int perf_type, int config) {
	struct perf_event_attr event = {
		.type = perf_type,
		.size = sizeof(struct perf_event_attr),
		.config = config,
		.disabled = true,
		.exclude_kernel = true,
		.exclude_hv = true
	};
	int fd = perf_event_open(&event, 0, -1, -1, 0);

	if (fd == -1) {
		log_fatal("cannot open perf_event fd: %s", strerror(errno));
	}

	return fd;
}

static void event_fd_start(int fd) {
	ioctl(fd, PERF_EVENT_IOC_RESET, 0);
	ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
}

static void event_fd_stop(int fd) {
	ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
}

static uint64_t event_fd_read(int fd) {
	uint64_t result;
	assert(read(fd, &result, sizeof(result)) == sizeof(result));
	return result;
}

measurement* measurement_begin() {
	measurement* m = malloc(sizeof(measurement));
	m->fds = calloc(COUNT_OF(counters), sizeof(int));
	assert(m->fds);
	for (uint64_t i = 0; i < COUNT_OF(counters); ++i) {
		m->fds[i] = event_fd_open(counters[i].perf_type,
				counters[i].perf_config);
	}
	for (uint64_t i = 0; i < COUNT_OF(counters); ++i) {
		event_fd_start(m->fds[i]);
	}
	return m;
}

measurement_results* measurement_end(measurement* m) {
	for (uint64_t i = 0; i < COUNT_OF(counters); ++i) {
		event_fd_stop(m->fds[i]);
	}
	measurement_results* results = malloc(sizeof(measurement_results));
	assert(results);
	results->counters = calloc(COUNT_OF(counters), sizeof(uint64_t));
	assert(results->counters);
	for (uint64_t i = 0; i < COUNT_OF(counters); ++i) {
		results->counters[i] = event_fd_read(m->fds[i]);
		close(m->fds[i]);
	}

	free(m->fds);
	free(m);

	return results;
}

json_t* measurement_results_to_json(measurement_results* results) {
	json_t* result = json_object();
	for (uint64_t i = 0; i < COUNT_OF(counters); ++i) {
		json_object_set_new(result,
				counters[i].name,
				json_integer(results->counters[i]));
	}
	return result;
}

void measurement_results_release(measurement_results* results) {
	free(results->counters);
	free(results);
}
