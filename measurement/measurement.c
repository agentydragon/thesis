#include "measurement.h"

#include "../log/log.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <assert.h>

// TODO: PERF_COUNT_SW_ALIGNMENT_FAULTS
// TODO: use perf_event groups later?

static int perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
	return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

static int event_fd_open(int config) {
	struct perf_event_attr event = {
		.type = PERF_TYPE_HARDWARE,
		.size = sizeof(struct perf_event_attr),
		.config = config,
		.disabled = true,
		.exclude_kernel = true,
		.exclude_hv = true
	};
	int fd = perf_event_open(&event, 0, -1, -1, 0);

	if (fd == -1) {
		log_fatal("Cannot open perf_event fd");
		exit(1);
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

struct measurement measurement_begin() {
	struct measurement measurement = (struct measurement) {
		.cache_references_fd = event_fd_open(PERF_COUNT_HW_CACHE_REFERENCES),
		.cache_misses_fd = event_fd_open(PERF_COUNT_HW_CACHE_MISSES),
	};

	event_fd_start(measurement.cache_references_fd);
	event_fd_start(measurement.cache_misses_fd);

	return measurement;
}

struct measurement_results measurement_end(struct measurement measurement) {
	event_fd_stop(measurement.cache_references_fd);
	event_fd_stop(measurement.cache_misses_fd);

	struct measurement_results results = {
		.cache_references = event_fd_read(measurement.cache_references_fd),
		.cache_misses = event_fd_read(measurement.cache_misses_fd)
	};

	close(measurement.cache_references_fd);
	close(measurement.cache_misses_fd);

	return results;
}
