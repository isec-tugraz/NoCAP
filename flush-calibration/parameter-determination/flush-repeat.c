#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "HappyUtils.h"
#include "PageUtils.h"
#include "rdtsc.h"

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <file_path> <output_file> [reps]\n",
				argv[0]);
		return EXIT_FAILURE;
	}

	char *file_path = argv[1];
	char *output_path = argv[2];

	uint64_t reps = 256;
	if (argc >= 4) {
		reps = (uint64_t)auto_strtoll(argv[3]);

		if (reps == 0) {
			reps = 256;
		}
	}

	struct stat st;
	if (stat(file_path, &st) == -1) {
		perror("Unable to get file size");
		return EXIT_FAILURE;
	}

	uint64_t file_pages = st.st_size / page_size;

	fprintf(stdout, "Number of File Pages: %lu\n", file_pages);

	int fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		return EXIT_FAILURE;
	}

	FILE *out = fopen(output_path, "w");
	if (!out) {
		perror("fopen output");
		close(fd);
		return EXIT_FAILURE;
	}

	fprintf(out, "NumPages,Average,Max,Min,StdDev,StdErr\n");

	char byte;

	uint64_t **per_pages_tsc_diffs = malloc(sizeof(uint64_t *) * file_pages);
	if (per_pages_tsc_diffs == NULL) {
		perror("malloc");
		return EXIT_FAILURE;
	}

	for (uint64_t n_pages = 0; n_pages < file_pages; ++n_pages) {
		per_pages_tsc_diffs[n_pages] = malloc(sizeof(uint64_t) * reps);
		if (per_pages_tsc_diffs[n_pages] == NULL) {
			perror("malloc");
			return EXIT_FAILURE;
		}

		memset(per_pages_tsc_diffs[n_pages], 0, reps * sizeof(uint64_t));
	}

	// we do the entire shebang reps times
	for (uint64_t reps_i = 0; reps_i < reps; ++reps_i) {
		// For 0 to file_pages, we're going to read-in and then time(flush-out)
		for (uint64_t n_pages = 0; n_pages < file_pages; ++n_pages) {
			progress_bar(reps_i * file_pages + n_pages, reps * file_pages);

			// Step 1: Read the pages
			for (uint64_t i = 0; i < n_pages; ++i) {
				off_t ret = lseek(fd, i * page_size, SEEK_SET);
				if (ret == -1) {
					perror("lseek");
					return EXIT_FAILURE;
				}

				if (read(fd, &byte, 1) == -1) {
					fprintf(stderr, "ERR: N_Pages: %lu, Rep: %lu, Page: %lu\n",
							n_pages, reps_i, i);
					perror("read");
					return EXIT_FAILURE;
				}
			}

			// Step 2: Flush
			uint64_t start = rdtsc();
			posix_fadvise(fd, 0, n_pages * page_size, POSIX_FADV_DONTNEED);
			uint64_t stop = rdtsc();

			per_pages_tsc_diffs[n_pages][reps_i] = stop - start;
		}
	}

	// Computing avg, min, max, stddev, stderr
	for (uint64_t n_pages = 0; n_pages < file_pages; ++n_pages) {

		double sum = 0.0;
		uint64_t max = 0;
		uint64_t min = UINT64_MAX;

		for (uint64_t r = 0; r < reps; ++r) {
			uint64_t v = per_pages_tsc_diffs[n_pages][r];

			sum += (double)v;

			if (v > max) {
				max = v;
			}

			if (v < min) {
				min = v;
			}
		}

		double average = sum / (double)reps;

		double variance = 0.0;
		for (uint64_t r = 0; r < reps; ++r) {
			double diff = (double)per_pages_tsc_diffs[n_pages][r] - average;

			variance += diff * diff;
		}

		variance /= (double)reps;

		double stddev = sqrt(variance);
		double std_err = stddev / sqrt((double)reps);

		fprintf(out, "%lu,%f,%lu,%lu,%f,%f\n", n_pages, average, max, min,
				stddev, std_err);
	}

	for (uint64_t n_pages = 0; n_pages < file_pages; ++n_pages) {
		free(per_pages_tsc_diffs[n_pages]);
	}

	free(per_pages_tsc_diffs);
	fclose(out);
	close(fd);

	return EXIT_SUCCESS;
}