#ifndef HAPPYUTILS_H
#define HAPPYUTILS_H

#include <inttypes.h>

void get_terminal_width() __attribute__((constructor));
void progress_bar(uint32_t current, uint32_t total);
int64_t auto_strtoll(const char *str);
long timespec_difference_ns(const struct timespec *start,
							const struct timespec *end);
struct timespec timespec_difference(const struct timespec start,
									const struct timespec end);

#endif // HAPPYUTILS_H