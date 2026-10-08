# arena

A high-performance, contiguous region-based memory allocator (Arena / Bump Allocator) written in C17, featuring power-of-2 alignment guarantees, integer overflow protection, and $O(1)$ bulk deallocation.

---

## Overview & Architecture

General-purpose memory allocators like `malloc()` carry significant metadata overhead, bin-traversal latency, and fragmentation costs. In phased execution models (e.g., HTTP request lifecycles, compilers, database query execution), managing individual object lifecycles is inefficient.

`arena` solves this by pre-allocating a single contiguous slab of memory and satisfying allocation requests via **bump-pointer arithmetic**. Individual objects are not freed; instead, all memory is reclaimed instantaneously in $O(1)$ time by resetting the allocation offset.

### Memory Layout & Alignment

```text
[========================== Total Buffer Capacity (e.g., 8 MB) ==========================]
+--------------------+---------+----------------------+--------------------+-------------+
| Struct A (align 8) | PADDING | Struct B (align 16)  | Struct C (align 4) | FREE SPACE  |
+--------------------+---------+----------------------+--------------------+-------------+
^                              ^                                           ^             ^
|                              |                                           |             |
0                          prev_offset                                   offset       capacity
                                                                     (high-water)
```

### The Alignment Math
To avoid CPU alignment penalties (or hardware bus traps on architectures like ARM), allocations are aligned to powers of 2 ($a \in \{1, 2, 4, 8, 16, \dots\}$) using fast bitwise masking instead of costly integer modulo division:

$$\text{aligned\_addr} = (\text{addr} + a - 1) \ \& \ \sim(a - 1)$$

- **Masking:** Because $a$ is a power of 2, $a - 1$ produces a bitmask of all trailing bits to be cleared, and $\sim(a - 1)$ creates a mask that zeroes them out (rounding down).
- **Rounding Up:** Adding $a - 1$ prior to masking pushes any unaligned address to or past the next alignment boundary, without advancing addresses that are already aligned.
- **Padding:** The allocator calculates padding bytes $(\text{aligned\_addr} - \text{addr})$ and advances the cursor by $(\text{padding} + \text{size})$.

---

## API Contract

| Function | Time Complexity | Description |
| :--- | :---: | :--- |
| `arena_create(capacity)` | $O(1)$ | Allocates the `Arena` header and backing byte slab upfront. |
| `arena_alloc(arena, size, align)` | $O(1)$ | Advances cursor to aligned offset, validates bounds, and returns slice. |
| `arena_reset(arena)` | $O(1)$ | Resets allocation cursor to 0. Does not free buffer or reset high-water mark. |
| `arena_destroy(arena)` | $O(1)$ | Releases backing slab and header struct back to the operating system. |
| `arena_allocated(arena)` | $O(1)$ | Returns current active bytes in use. |
| `arena_capacity(arena)` | $O(1)$ | Returns total buffer capacity. |
| `arena_high_water_mark(arena)` | $O(1)$ | Returns peak memory utilization reached across resets. |

---

## Project Structure

```text
arena/
├── include/
│   └── arena.h         # Public interface, types, and contracts
├── src/
│   └── arena.c         # Allocator implementation & alignment engine
├── tests/
│   └── test_arena.c    # Unit tests (alignment, padding, OOM, reset, data integrity)
├── bench/
│   └── bench_arena.c   # High-throughput benchmark vs standard malloc/free
├── Makefile            # Strict warnings, debug/release modes, sanitizer flags
└── README.md
```

---

## Build & Test

The build system enforces strict compiler warnings (`-Wall -Wextra -Wpedantic`) and integrates AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).

### Requirements
- C17 compatible compiler (GCC or Clang)
- POSIX-compliant OS (for `clock_gettime(CLOCK_MONOTONIC)`)
- GNU Make

### Makefile Targets

```bash
# Run unit test suite under AddressSanitizer and UndefinedBehaviorSanitizer
make test

# Run high-volume performance benchmark (compiled with -O3 optimizations)
make bench

# Clean build artifacts
make clean
```

---

## Benchmark Results

Benchmark simulating **5,000,000 allocations** (50 request batches $\times$ 100,000 allocations of 32-byte objects with 8-byte alignment) compiled under `-O3 -DNDEBUG`:

```text
====================================================
Arena Allocator vs. malloc/free Benchmark
Total Allocations: 5000000 (50 batches of 100000)
Allocation Size  : 32 bytes (align 8)
====================================================

[malloc + free      ] Elapsed:   0.0700 s
[arena_alloc + reset] Elapsed:   0.0083 s (Peak memory: 3125 KB)

----------------------------------------------------
Arena Allocator was 8.45x faster than malloc/free!
====================================================
```

### Systems Analysis
1. **Allocator Overhead:** `malloc` maintains metadata per chunk, traverses free-list bins, and modifies allocator bookkeeping. In contrast, `arena_alloc` executes in ~3 to 5 machine instructions: an alignment mask, a subtraction bounds-check, and an integer addition.
2. **Elimination of Pointer Tracking:** Under `malloc/free`, callers must maintain separate arrays/data structures purely to track object pointers for cleanup. With an arena, individual tracking is eliminated—all memory is reclaimed in a single assignment (`offset = 0`).
3. **Cache Line Locality:** Allocations out of a contiguous slab reside in sequentially adjacent cache lines, maximizing hardware L1/L2 prefetcher hit rates.

---

## Limitations & Engineering Trade-offs

- **No Individual Free:** Memory cannot be freed on an object-by-object basis. Attempting to manage fine-grained object lifetimes with an arena will cause memory bloat until the next reset.
- **Fixed Capacity:** The current implementation uses a fixed backing slab. Requests exceeding the slab capacity fail safely with `NULL`. (A production extension is to chain multiple arena blocks together via a linked list).
- **Single-Threaded:** Designed for thread-local or single-worker usage (such as per-request or per-task contexts). Concurrent allocations require external synchronization.
