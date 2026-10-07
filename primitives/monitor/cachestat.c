/* Reports page presence in cache using the cachestat syscall.
 *
 * Args:
 * 		- /path/to/file
 * 		- pages of file (separated by space), optional: -1 or omitted
 * 		  entirely means "every page in the file"
 */

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
#include <unistd.h>

#include "CacheStat.h"
#include "GeneralUtils.h"
#include "PageUtils.h"

void exec_cachestat_pages(int fd, int64_t n_arg_pages, int64_t *pages)
{
	struct cachestat *cs =
		(struct cachestat *)malloc(n_arg_pages * sizeof(struct cachestat));

	for (int64_t i = 0; i < n_arg_pages; ++i) {
		struct cachestat_range cs_range = {.off = pages[i] * page_size,
										   .len = page_size};

		if (syscall(SYSCALL_CACHESTAT, fd, &cs_range, &cs[i], 0) == -1) {
			perror("cachestat failed :(");
		}
	}

	fprintf(stdout, "page, cache\n");
	for (int64_t i = 0; i < n_arg_pages; ++i) {
		fprintf(stdout, "%04ld,   %lu\n", pages[i], cs[i].nr_cache);
	}
}

void exec_cachestat_total(int fd, int64_t n_file_pages)
{
	struct cachestat *cs =
		(struct cachestat *)malloc(n_file_pages * sizeof(struct cachestat));

	// going page-by-page. you can do off 0, len 0 to get whole file stats,
	// but we'd like to see the page-by-page stats.
	for (int64_t i = 0; i < n_file_pages; ++i) {
		struct cachestat_range cs_range = {.off = i * page_size,
										   .len = page_size};

		if (syscall(SYSCALL_CACHESTAT, fd, &cs_range, &cs[i], 0) == -1) {
			perror("cachestat failed :(");
		}
	}

	uint64_t in_cache = 0;

	fprintf(stdout, "page, cache\n");
	for (int64_t i = 0; i < n_file_pages; ++i) {
		fprintf(stdout, "%04ld,   %lu\n", i, cs[i].nr_cache);

		in_cache += cs[i].nr_cache;
	}

	fprintf(stdout, "\nTotal: %ld\n", n_file_pages);
	fprintf(stdout, "In cache: %ld\n", in_cache);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file_path> [page_num]+...\n", argv[0]);
		fprintf(stderr, "       (omit page_num to check every page)\n");
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

	bool monitor_all = false;
	int64_t *pages = NULL;

	if (argc == 2) {
		// No page numbers on the command line: check the whole file.
		monitor_all = true;
		n_arg_pages = 0;
	} else {
		pages = parse_page_list(n_arg_pages, &argv[2], &monitor_all, n_file_pages);

		// We do things a little bit efficiently
		qsort(pages, n_arg_pages, sizeof(uint64_t), compare_uint64_t);
	}

	int fd = open(file_path, O_RDONLY);
	if (fd == -1) {
		perror("open");
		free(pages);
		return EXIT_FAILURE;
	}

	if (monitor_all) {
		exec_cachestat_total(fd, n_file_pages);
	} else {
		exec_cachestat_pages(fd, n_arg_pages, pages);
	}

	free(pages);

	return EXIT_SUCCESS;
}
