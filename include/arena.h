#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;          /* Pointer to raw byte buffer */
    size_t capacity;          /* Total size of the buffer in bytes */
    size_t offset;            /* Current allocation cursor (0 <= offset <= capacity) */
    size_t prev_offset;       /* Previous allocation offset (optional, for pop/debug) */
    size_t high_water_mark;   /* Peak offset ever reached */
} Arena;

/**
 * Creates and initializes a heap-allocated Arena.
 * @param capacity Total bytes to allocate upfront for the arena slab.
 * @return Pointer to initialized Arena, or NULL on allocation failure.
 */
Arena *arena_create(size_t capacity);

/**
 * Allocates a contiguous block of memory with a given alignment.
 * @param arena Pointer to the arena.
 * @param size Number of bytes to allocate.
 * @param alignment Must be a power of 2 (1, 2, 4, 8, 16, etc.).
 * @return Pointer to aligned memory, or NULL if out of memory / invalid inputs.
 */
void *arena_alloc(Arena *arena, size_t size, size_t alignment);

/**
 * Reclaims all memory in O(1) by resetting the allocation offset to 0.
 * Does NOT free the backing buffer or reset high_water_mark.
 */
void arena_reset(Arena *arena);

/**
 * Frees the backing buffer and the Arena struct itself back to the OS.
 * Safe to pass NULL.
 */
void arena_destroy(Arena *arena);

/* --- Statistics / Inspection Functions --- */

/** Returns current bytes in active use. */
size_t arena_allocated(const Arena *arena);

/** Returns total capacity of the arena in bytes. */
size_t arena_capacity(const Arena *arena);

/** Returns peak memory usage reached before resets. */
size_t arena_high_water_mark(const Arena *arena);

#endif
