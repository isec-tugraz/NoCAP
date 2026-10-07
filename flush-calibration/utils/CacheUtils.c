#include "CacheUtils.h"

inline void maccess(void *p)
{
	asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
}

inline void mfence() { asm volatile("mfence"); }

inline void clflush(void *p) {
    asm volatile("clflush (%0)" : : "r"(p) : "memory");
}