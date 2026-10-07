/* This code was generously and shamelessly stolen from DRAMA
 *
 * Step 1: run load_pagemap()
 * Step 2: run get_physical_addr()
 * Step 3: ???
 * Step 4: profit
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "PageUtils.h"

int64_t process_pagemap = -1;

int8_t load_pagemap()
{
	if (load_page_size() != 0)
		return -1;

	process_pagemap = open("/proc/self/pagemap", O_RDONLY);
	assert(process_pagemap >= 0);

	return 0;
}

int8_t close_pagemap()
{
	if (process_pagemap != -1)
		close(process_pagemap);

	return 0;
}

uint64_t GetPageFrameNumber(int pagemap, uint8_t *virtual_address)
{
	// Read the entry in the pagemap.
	uint64_t value;

	int got = pread(pagemap, &value, 8,
					((uintptr_t)(virtual_address) / page_size) * 8);
	assert(got == 8);

	uint64_t page_frame_number = value & ((1ULL << 54) - 1);
	return page_frame_number;
}

// Extract the physical page number from a Linux /proc/PID/pagemap entry.
static uint64_t frame_number_from_pagemap(uint64_t value)
{
	return value & ((1ULL << 54) - 1);
}

uint64_t get_physical_addr(uint64_t virtual_addr)
{
	// did you run load_pagemap()?
	assert(process_pagemap != -1);

	uint64_t value;
	off_t offset = (virtual_addr / page_size) * sizeof(value);
	int got = pread(process_pagemap, &value, sizeof(value), offset);
	assert(got == 8);

	// Check the "page present" flag.
	if (!(value & (1ULL << 63))) {
		return -1;
	}

	uint64_t frame_num = frame_number_from_pagemap(value);
	return (frame_num * page_size) | (virtual_addr & (page_size - 1));
}