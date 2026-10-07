#ifndef CACHEUTILS_H
#define CACHEUTILS_H

#include <inttypes.h>

#define KiB 1024
#define MiB 1024 * KiB
#define GiB 1024 * MiB

void clflush(void *p);
void maccess(void *p);
void mfence();

#endif // CACHEUTILS_H