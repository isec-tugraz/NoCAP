#ifndef MSRUTILS_H
#define MSRUTILS_H

#include <inttypes.h>

// reads `processor_id`'s `msr` to `value`
int32_t rdmsr(uint64_t msr, uint64_t processor_id, uint64_t *value);

// writes `value` to `processor_id`'s `msr`
uint64_t wrmsr(uint64_t msr, uint64_t processor_id, uint64_t value);

#endif // MSRUTILS_H