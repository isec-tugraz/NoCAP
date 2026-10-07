#ifndef CACHESTAT_H
#define CACHESTAT_H

#include <inttypes.h>

// super hardcoded, but we live life on da line
#define SYSCALL_CACHESTAT 451

/* Since the compiler doesn't know about these structures, we're going to have
 * to manually define them.
 *
 * Reference:
 * https://lwn.net/ml/linux-kernel/20230503013608.2431726-3-nphamcs@gmail.com/
 */
struct cachestat_range {
	uint64_t off;
	uint64_t len;
};

struct cachestat {
	uint64_t nr_cache;
	uint64_t nr_dirty;
	uint64_t nr_writeback;
	uint64_t nr_evicted;
	uint64_t nr_recently_evicted;
};

#endif//CACHESTAT_H
