# Minimal subset of the eviction-notice primitives: only the 4 programs
# needed for NoCAP testing
#
#   make flush                        -> build/flush
#   make read-bypass-backward-reading -> build/read-bypass-backward-reading
#   make monitor-cachestat            -> build/monitor-cachestat
#   make                              -> builds all 4 above
all: flush read-bypass-backward-reading monitor-cachestat

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter

BUILD_DIR := ./build
builddir := @mkdir -p $(BUILD_DIR)

LOC_INCLUDE_DIR := ./include
LOC_INCLUDES := $(shell find $(LOC_INCLUDE_DIR)/ -name '*.h')

LOC_UTILS_DIR := ./utils
LOC_UTILS := $(shell find $(LOC_UTILS_DIR)/ -name '*.c')

################################################################################
flush: flush/flush.c $(LOC_INCLUDES) $(LOC_UTILS)
	$(builddir)
	$(CC) flush/flush.c                                                        \
		$(LOC_UTILS)                                                           \
		$(CFLAGS)                                                              \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-o $(BUILD_DIR)/flush

################################################################################
read-bypass-backward-reading: reload/read-bypass-backward-reading.c $(LOC_INCLUDES) $(LOC_UTILS)
	$(builddir)
	$(CC) reload/read-bypass-backward-reading.c                                \
		$(LOC_UTILS)                                                           \
		$(CFLAGS)                                                              \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-o $(BUILD_DIR)/read-bypass-backward-reading

################################################################################
monitor-cachestat: monitor/cachestat.c $(LOC_INCLUDES) $(LOC_UTILS)
	$(builddir)
	$(CC) monitor/cachestat.c                                                  \
		$(LOC_UTILS)                                                           \
		$(CFLAGS)                                                              \
		-I$(LOC_INCLUDE_DIR)                                                   \
		-o $(BUILD_DIR)/monitor-cachestat

.PHONY: clean flush read-bypass-backward-reading monitor-cachestat

clean:
	rm -f $(BUILD_DIR)/* $(LOC_INCLUDE_DIR)/*.h.gch
