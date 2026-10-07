#ifndef RDEETSC_H
#define RDEETSC_H

inline uint64_t rdtsc()
{
    uint64_t a, d;

    asm volatile("mfence\n\t");
    asm volatile("rdtsc\n\t" : "=a"(a), "=d"(d));

    a = (d << 32) | a;

    asm volatile("mfence\n");

    return a;
}

#endif //RDEETSC_H