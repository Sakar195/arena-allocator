CC ?= gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c17 -Iinclude

# Debug flags (Sanitizers + Debug symbols + No optimizations)
DEBUG_FLAGS = -O0 -g -fsanitize=address,undefined -DDEBUG

# Release flags (High optimization + No assertions)
RELEASE_FLAGS = -O3 -DNDEBUG

SRC = src/arena.c

.PHONY: all debug release test bench basic clean

all: test

# Debug build target
debug: CFLAGS += $(DEBUG_FLAGS)
debug: test basic

# Release build target
release: CFLAGS += $(RELEASE_FLAGS)
release: bench

# Unit tests run with sanitizers always enabled
test: tests/test_arena.c $(SRC)
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $^ -o run_tests
	./run_tests

# Benchmark runs with full release optimizations and NO sanitizers
bench: bench/bench_arena.c $(SRC)
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) $^ -o run_bench
	./run_bench

clean:
	rm -f src/*.o run_tests run_basic run_bench

