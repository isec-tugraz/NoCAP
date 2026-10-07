all: flush-repeat 

CC := gcc
CFLAGS := -O2 -Wall -Wextra -pedantic -Wno-unused-parameter

BUILD_DIR := ./build
builddir := @mkdir -p $(BUILD_DIR)

INCLUDE_DIR := ../utils/include
INCLUDES := $(shell find $(INCLUDE_DIR)/ -name '*.h')

UTILS_DIR := ../utils
UTILS := $(shell find $(UTILS_DIR)/ -name '*.c')

flush-repeat: flush-repeat.c $(INCLUDES) $(UTILS)
	$(builddir)
	$(CC) $(CFLAGS) \
		-I$(INCLUDE_DIR) \
		-o $(BUILD_DIR)/flush-repeat \
		$(UTILS) flush-repeat.c -lm

.PHONY: clean 

clean:
	rm -f $(BUILD_DIR)/*

