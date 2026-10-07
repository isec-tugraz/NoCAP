#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "MsrUtils.h"

static int32_t open_msr_driver(uint64_t processor_id)
{
	int32_t msr_fd;
	char msr_path[128];

	sprintf(msr_path, "/dev/cpu/%lu/msr", processor_id);
	msr_fd = open(msr_path, O_RDWR);
	if (msr_fd < 0) {
		fprintf(stderr, "Failed to open: %s\n", msr_path);
		perror("open");
		return -1;
	}

	return msr_fd;
}

int32_t rdmsr(uint64_t msr, uint64_t processor_id, uint64_t *value)
{
	int msr_fd = open_msr_driver(processor_id);
	if (msr_fd == -1)
		return -1;

	if (pread(msr_fd, value, sizeof(*value), msr) != sizeof(*value)) {
		fprintf(stderr, "Failed to rdmsr: %lu (0x%lx)\n", msr, msr);
		perror("pread");
		close(msr_fd);
		return -1;
	}

	close(msr_fd);
	return 0;
}

uint64_t wrmsr(uint64_t msr, uint64_t processor_id, uint64_t value)
{
	int msr_fd = open_msr_driver(processor_id);
	if (msr_fd == -1)
		return -1;

	if (pwrite(msr_fd, &value, sizeof(value), msr) != sizeof(value)) {
		fprintf(stderr, "Failed to rdmsr: %lu (0x%lx)\n", msr, msr);
		perror("pread");
		close(msr_fd);
		return -1;
	}

	close(msr_fd);
	return 0;
}