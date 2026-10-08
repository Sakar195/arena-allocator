#include "arena.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * Creates and initializes a heap-allocated Arena.
 * @param capacity Total bytes to allocate upfront for the arena slab.
 * @return Pointer to initialized Arena, or NULL on allocation failure.
 */
Arena *arena_create(size_t capacity) {
  if (capacity == 0)
    return NULL;
  Arena *arena = malloc(sizeof(Arena));

  if (arena == NULL) {
    return NULL;
  }

  uint8_t *memory = malloc(capacity);
  if (memory == NULL) {
    free(arena);
    return NULL;
  }
  arena->buffer = memory;
  arena->capacity = capacity;
  arena->offset = 0;
  arena->prev_offset = 0;
  arena->high_water_mark = 0;
  return arena;
}

/**
 * Frees the backing buffer and the Arena struct itself back to the OS.
 * Safe to pass NULL.
 */
void arena_destroy(Arena *arena) {
  if (arena == NULL)
    return;
  free(arena->buffer);
  free(arena);
}

/**
 * Allocates a contiguous block of memory with a given alignment.
 * @param arena Pointer to the arena.
 * @param size Number of bytes to allocate.
 * @param alignment Must be a power of 2 (1, 2, 4, 8, 16, etc.).
 * @return Pointer to aligned memory, or NULL if out of memory / invalid inputs.
 */
void *arena_alloc(Arena *arena, size_t size, size_t alignment) {
  if (arena == NULL || size == 0)
    return NULL;
  // Making sure alignment is a power of 2 before proceeding further with
  // bitwise check
  if (alignment == 0 || (alignment & (alignment - 1)) != 0)
    return NULL;

  uint8_t *current_ptr = arena->buffer + arena->offset;
  uintptr_t current_addr = (uintptr_t)current_ptr;
  uintptr_t aligned_addr = (current_addr + alignment -1) & ~(uintptr_t)(alignment - 1);
  size_t padding = (size_t)(aligned_addr - current_addr);

  size_t available = arena->capacity - arena->offset;
  if(padding + size > available || padding + size < size){
    return NULL;
  }
  arena->prev_offset = arena->offset;
  arena->offset += padding + size;
  if(arena->offset > arena->high_water_mark){
    arena->high_water_mark = arena->offset;
  }
  return (void *)aligned_addr;
}

/**
 * Reclaims all memory in O(1) by resetting the allocation offset to 0.
 * Does NOT free the backing buffer or reset high_water_mark.
 */
void arena_reset(Arena *arena){
  if(arena==NULL) return;
  arena->offset=0;
  arena->prev_offset=0;
}

/* --- Statistics / Inspection Functions --- */

/** Returns current bytes in active use. */
size_t arena_allocated(const Arena *arena) { return arena ? arena->offset : 0; }

/** Returns total capacity of the arena in bytes. */
size_t arena_capacity(const Arena *arena) {
  return arena ? arena->capacity : 0;
}

/** Returns peak memory usage reached before resets. */
size_t arena_high_water_mark(const Arena *arena) {
  return arena ? arena->high_water_mark : 0;
}
