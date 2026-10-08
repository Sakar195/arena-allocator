#define _POSIX_C_SOURCE 199309L
#include "arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define NUM_ITERATIONS 50
#define ALLOCS_PER_ITERATION 100000UL // 100,000 objects per request
#define ALLOC_SIZE 32                 // 32-byte object
#define ALLOC_ALIGN 8                 // 8-byte alignment

static double get_elapsed_seconds(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1e9;
}

int main(void) {
    struct timespec start, end;
    double time_malloc = 0.0;
    double time_arena = 0.0;

    const size_t total_allocs = NUM_ITERATIONS * ALLOCS_PER_ITERATION;

    printf("====================================================\n");
    printf("Arena Allocator vs. malloc/free Benchmark\n");
    printf("Total Allocations: %lu (%d batches of %lu)\n",
           total_allocs, NUM_ITERATIONS, ALLOCS_PER_ITERATION);
    printf("Allocation Size  : %d bytes (align %d)\n", ALLOC_SIZE, ALLOC_ALIGN);
    printf("====================================================\n\n");

    /* -----------------------------------------------------------
     * Experiment 1: Standard malloc() + free()
     * ----------------------------------------------------------- */
    {
        // We need an array to hold the pointers so we can free them later.
        // (Notice: malloc forces you to waste memory storing pointer lists!)
        void **ptrs = malloc(ALLOCS_PER_ITERATION * sizeof(void *));
        if (!ptrs) {
            fprintf(stderr, "Failed to allocate pointer array\n");
            return 1;
        }

        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
            // 1. Allocate individual objects
            for (size_t i = 0; i < ALLOCS_PER_ITERATION; i++) {
                ptrs[i] = malloc(ALLOC_SIZE);
                // Write data so the compiler cannot optimize the allocation away
                ((int *)ptrs[i])[0] = (int)i;
            }

            // 2. Free every single object individually
            for (size_t i = 0; i < ALLOCS_PER_ITERATION; i++) {
                free(ptrs[i]);
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        time_malloc = get_elapsed_seconds(start, end);
        free(ptrs);

        printf("[malloc + free      ] Elapsed: %8.4f s\n", time_malloc);
    }

    /* -----------------------------------------------------------
     * Experiment 2: Arena Allocator (arena_alloc + arena_reset)
     * ----------------------------------------------------------- */
    {
        // Sizing the arena: 100,000 * 32 bytes = 3.2 MB. We give it 8 MB.
        const size_t arena_capacity_bytes = 8 * 1024 * 1024;
        Arena *arena = arena_create(arena_capacity_bytes);
        if (!arena) {
            fprintf(stderr, "Failed to create arena\n");
            return 1;
        }

        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
            // 1. Allocate out of the arena
            for (size_t i = 0; i < ALLOCS_PER_ITERATION; i++) {
                void *ptr = arena_alloc(arena, ALLOC_SIZE, ALLOC_ALIGN);
                // Write data so compiler cannot optimize the allocation away
                ((int *)ptr)[0] = (int)i;
            }

            // 2. Instant O(1) bulk free for the entire batch!
            arena_reset(arena);
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        time_arena = get_elapsed_seconds(start, end);

        printf("[arena_alloc + reset] Elapsed: %8.4f s (Peak memory: %zu KB)\n",
               time_arena, arena_high_water_mark(arena) / 1024);

        arena_destroy(arena);
    }

    /* -----------------------------------------------------------
     * Summary Comparison
     * ----------------------------------------------------------- */
    printf("\n----------------------------------------------------\n");
    if (time_arena > 0.0) {
        double speedup = time_malloc / time_arena;
        printf("Arena Allocator was %.2fx faster than malloc/free!\n", speedup);
    }
    printf("====================================================\n");

    return 0;
}
