/* Flush specific pages of a file, reporting how long each flush took.
 *
 * Args:
 * 		- /path/to/file
 * 		- pages of file (separated by space), optional: -1 or omitted
 * 		  entirely means "every page in the file"
 */

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
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

#include "GeneralUtils.h"
#include "PageUtils.h"

// clang-format off
#ifndef FLUSH_THRESHOLD
    #define FLUSH_THRESHOLD 0.000000900
#endif
// clang-format on

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file_path> [page_num]+...\n", argv[0]);
		fprintf(stderr, "       (omit page_num to flush every page)\n");
		return EXIT_FAILURE;
	}

	char *file_path = argv[1];

	struct stat st;
	if (stat(file_path, &st) == -1) {
		perror("Unable to get file size");
		return EXIT_FAILURE;
	}

	int64_t file_size = st.st_size;
	int64_t n_file_pages = file_size / page_size;
	if (!n_file_pages)
		n_file_pages = 1;

	int64_t n_arg_pages = argc - 2;

	bool flush_all = false;
	int64_t *pages = NULL;

	if (argc == 2) {
		// No page numbers on the command line: flush the whole file.
		flush_all = true;
		n_arg_pages = 0;
	} else {
		pages = parse_page_list(n_arg_pages, &argv[2], &flush_all, n_file_pages);

		// We do things a little bit efficiently
		qsort(pages, n_arg_pages, sizeof(uint64_t), compare_uint64_t);
	}

	int fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		free(pages);
		return EXIT_FAILURE;
	}

	int total_pages = flush_all ? n_file_pages : n_arg_pages;
	struct timespec *times = malloc(total_pages * sizeof(struct timespec));
	struct timespec end_time;

	for (int64_t i = 0; i < total_pages; ++i) {
		int64_t page_index = flush_all ? i : pages[i];

		lseek(fd, page_index * page_size, SEEK_SET);

		clock_gettime(CLOCK_MONOTONIC_RAW, &times[i]);
		posix_fadvise(fd, page_index * page_size, page_size,
					  POSIX_FADV_DONTNEED);
		clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);

		times[i].tv_sec = end_time.tv_sec - times[i].tv_sec;
		times[i].tv_nsec = end_time.tv_nsec - times[i].tv_nsec;

		if (times[i].tv_nsec < 0) {
			times[i].tv_sec--;
			times[i].tv_nsec += 1e9;
		}
	}

	fprintf(stdout, "page, time [s]\n");
	for (int64_t i = 0; i < total_pages; ++i) {
		fprintf(stdout, "%04ld, %ld.%09ld\n", i, times[i].tv_sec,
				times[i].tv_nsec);
	}

	free(pages);

	return EXIT_SUCCESS;
}
