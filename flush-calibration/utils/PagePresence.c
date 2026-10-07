#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "PageUtils.h"

int8_t page_in_cache(int fd, int64_t page_num)
{
	char buffer = 0;
	struct iovec iov[1];
	iov[0].iov_base = &buffer;
	iov[0].iov_len = 1;

	lseek(fd, 0, SEEK_SET);
	ssize_t ret = preadv2(fd, iov, 1, page_num * page_size, RWF_NOWAIT);

	int8_t in_cache = 2;

	if (ret < 0) {
		if (errno == EAGAIN) {
			// not in cache
			in_cache = 0;
		} else {
			perror("preadv2 failed");
		}
	} else if (ret == 1) {
		// We asked to read one byte, one byte was read
		in_cache = 1;
	} else {
		// something went wrong
		in_cache = -1;
	}

	return in_cache;
}

void page_presence_bmp(int fd, uint64_t num_file_pages)
{
	for (uint64_t page_num = 0; page_num < num_file_pages; ++page_num) {
		fprintf(stdout, "%d", page_in_cache(fd, page_num));

		if (page_num % 8 == 7)
			fprintf(stdout, " ");
	}

	fprintf(stdout, "\n");

	return;
}

void page_presence_line(int fd, uint64_t num_file_pages)
{
	for (uint64_t page_num = 0; page_num < num_file_pages; ++page_num) {
		fprintf(stdout, "%lu: %d\n", page_num, page_in_cache(fd, page_num));
	}

	return;
}