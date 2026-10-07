/* This code was generously and shamelessly stolen from DRAMA
 *
 * Step 1: run load_pagemap()
 * Step 2: run get_physical_addr()
 * Step 3: ???
 * Step 4: profit
 */
#ifndef PAGEUTILS_H
#define PAGEUTILS_H

#include <inttypes.h>

extern int64_t process_pagemap;
extern uint64_t page_size;

int8_t load_page_size() __attribute__((constructor));
int8_t load_pagemap();
int8_t close_pagemap();

uint64_t get_physical_addr(uint64_t virtual_addr);
uint64_t GetPageFrameNumber(int pagemap, uint8_t *virtual_address);

void page_presence_bmp(int fd, uint64_t num_file_pages);
void page_presence_line(int fd, uint64_t num_file_pages);

/* allocate number_pages of OS_PAGESIZE at array
 * use memset(array, 'A', number_pages * page_size);
 */
uint8_t *get_aligned_pages(uint32_t number_pages);

#endif // PAGEUTILS_H