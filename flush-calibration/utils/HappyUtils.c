#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include "HappyUtils.h"

static uint32_t bar_width = 0;

void progress_bar(uint32_t current, uint32_t total)
{
	float progress = (float)current / total;

	uint32_t pos = bar_width * progress;

	fprintf(stdout, "[");
	for (uint32_t i = 0; i < bar_width; ++i) {
		if (i < pos) {
			printf("=");
		} else {
			printf(" ");
		}
	}
	fprintf(stdout, "] %.2f%%\r", progress * 100);
	fflush(stdout);
}

void get_terminal_width()
{
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
		perror("ioctl");
		exit(-1);
	}
	bar_width = w.ws_col / 2;
}

int64_t auto_strtol(const char *str)
{
	while (isspace(*str)) {
		str++;
	}

	int base = 10;
	if (*str == '0') {
		if (*(str + 1) == 'x' || *(str + 1) == 'X') {
			base = 16;
			str += 2;
		} else {
			base = 8;
			str += 1;
		}
	}

	char *endptr;
	int64_t result = strtoll(str, &endptr, base);

	if (endptr == str) {
		return 0;
	}

	return result;
}

int64_t auto_strtoll(const char *str)
{
	char *endptr;

	// Base 0 does automatic base detection POGCHAMP + HELL YEAH
	int64_t value = strtoll(str, &endptr, 0);

	if (errno == ERANGE) {
		perror("Value out of range");
		return 0;
	}
	if (errno == EINVAL) {
		fprintf(stderr, "No digits or weird base bruv???\n");
		return 0;
	}

	return value;
}

long timespec_difference_ns(const struct timespec *start,
							const struct timespec *end)
{
	long seconds = end->tv_sec - start->tv_sec;
	long nanoseconds = end->tv_nsec - start->tv_nsec;

	if (nanoseconds < 0) {
		seconds--;
		nanoseconds += 1000000000;
	}

	long retval = seconds * 1000000000 + nanoseconds;

	return retval;
}

struct timespec timespec_difference(const struct timespec start,
									const struct timespec end)
{
	struct timespec result;

	long seconds = end.tv_sec - start.tv_sec;
	long nanoseconds = end.tv_nsec - start.tv_nsec;

	if (nanoseconds < 0) {
		seconds--;
		nanoseconds += 1e9;
	}

	result.tv_sec = seconds;
	result.tv_nsec = nanoseconds;

	return result;
}