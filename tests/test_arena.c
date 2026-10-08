#include "arena.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_create_and_destroy(void) {
    printf("Running: test_create_and_destroy... ");

    assert(arena_create(0) == NULL);

    arena_destroy(NULL);

    Arena *a = arena_create(1024);
    assert(a != NULL);
    assert(arena_capacity(a) == 1024);
    assert(arena_allocated(a) == 0);
    assert(arena_high_water_mark(a) == 0);

    arena_destroy(a);
    printf("PASSED\n");
}

static void test_invalid_alloc_inputs(void) {
    printf("Running: test_invalid_alloc_inputs... ");

    Arena *a = arena_create(1024);
    assert(a != NULL);

    // NULL arena
    assert(arena_alloc(NULL, 16, 8) == NULL);

    // Size 0
    assert(arena_alloc(a, 0, 8) == NULL);

    // Non-power-of-2 alignments
    assert(arena_alloc(a, 16, 0) == NULL);
    assert(arena_alloc(a, 16, 3) == NULL);
    assert(arena_alloc(a, 16, 5) == NULL);
    assert(arena_alloc(a, 16, 7) == NULL);
    assert(arena_alloc(a, 16, 15) == NULL);

    // Arena state must remain unchanged after bad requests
    assert(arena_allocated(a) == 0);

    arena_destroy(a);
    printf("PASSED\n");
}

static void test_alignments_and_padding(void) {
    printf("Running: test_alignments_and_padding... ");

    Arena *a = arena_create(4096);
    assert(a != NULL);

    // Test multiple power-of-2 alignments
    size_t alignments[] = {1, 2, 4, 8, 16, 32, 64};
    for (size_t i = 0; i < sizeof(alignments) / sizeof(alignments[0]); i++) {
        size_t align = alignments[i];
        void *ptr = arena_alloc(a, 17, align); // 17 is prime, ensures odd sizes
        assert(ptr != NULL);
        assert(((uintptr_t)ptr % align) == 0); // Must be cleanly divisible!
    }

    arena_reset(a);
    assert(arena_allocated(a) == 0);

    // Trace exact padding:
    // 1. Allocate 1 byte with 1-byte alignment -> offset becomes 1
    void *p1 = arena_alloc(a, 1, 1);
    assert(p1 != NULL);
    assert(arena_allocated(a) == 1);

    // 2. Allocate 8 bytes with 8-byte alignment:
    // Address 1 requires 7 bytes of padding to reach 8.
    // Total offset should become: 1 + 7 (pad) + 8 (size) = 16 bytes!
    void *p2 = arena_alloc(a, 8, 8);
    assert(p2 != NULL);
    assert(((uintptr_t)p2 % 8) == 0);
    assert(arena_allocated(a) == 16);

    arena_destroy(a);
    printf("PASSED\n");
}

static void test_data_integrity(void) {
    printf("Running: test_data_integrity... ");

    Arena *a = arena_create(2048);
    assert(a != NULL);

    // Allocate an int, a double, and a string
    int *my_int = (int *)arena_alloc(a, sizeof(int), _Alignof(int));
    double *my_double = (double *)arena_alloc(a, sizeof(double), _Alignof(double));
    char *my_str = (char *)arena_alloc(a, 32, _Alignof(char));

    assert(my_int != NULL && my_double != NULL && my_str != NULL);

    // Write values
    *my_int = 42;
    *my_double = 3.14159265;
    strcpy(my_str, "Systems programming in C");

    // Verify values didn't corrupt each other
    assert(*my_int == 42);
    assert(*my_double == 3.14159265);
    assert(strcmp(my_str, "Systems programming in C") == 0);

    arena_destroy(a);
    printf("PASSED\n");
}

static void test_out_of_memory(void) {
    printf("Running: test_out_of_memory... ");

    // Small arena of 64 bytes
    Arena *a = arena_create(64);
    assert(a != NULL);

    // Requesting more than total capacity
    assert(arena_alloc(a, 128, 8) == NULL);
    assert(arena_allocated(a) == 0);

    // Allocate 48 bytes
    void *p1 = arena_alloc(a, 48, 8);
    assert(p1 != NULL);
    assert(arena_allocated(a) == 48);

    // Remaining space is 16 bytes. Requesting 24 bytes must fail safely
    assert(arena_alloc(a, 24, 8) == NULL);
    // Allocated bytes should remain at 48
    assert(arena_allocated(a) == 48);

    // Allocating exactly 16 bytes should succeed
    void *p2 = arena_alloc(a, 16, 8);
    assert(p2 != NULL);
    assert(arena_allocated(a) == 64);

    // Arena is now completely full
    assert(arena_alloc(a, 1, 1) == NULL);

    arena_destroy(a);
    printf("PASSED\n");
}

static void test_reset_and_high_water_mark(void) {
    printf("Running: test_reset_and_high_water_mark... ");

    Arena *a = arena_create(1000);
    assert(a != NULL);

    // 1. Allocate 300 bytes
    arena_alloc(a, 300, 1);
    assert(arena_allocated(a) == 300);
    assert(arena_high_water_mark(a) == 300);

    // 2. Reset arena
    arena_reset(a);
    assert(arena_allocated(a) == 0);
    assert(arena_high_water_mark(a) == 300); // Preserved!

    // 3. Allocate 150 bytes (below previous peak)
    arena_alloc(a, 150, 1);
    assert(arena_allocated(a) == 150);
    assert(arena_high_water_mark(a) == 300); // Should NOT drop!

    // 4. Allocate another 250 bytes (total 400, exceeds old peak of 300)
    arena_alloc(a, 250, 1);
    assert(arena_allocated(a) == 400);
    assert(arena_high_water_mark(a) == 400); // Updated to new peak!

    arena_destroy(a);
    printf("PASSED\n");
}

int main(void) {
    printf("=== Starting Arena Allocator Tests ===\n");

    test_create_and_destroy();
    test_invalid_alloc_inputs();
    test_alignments_and_padding();
    test_data_integrity();
    test_out_of_memory();
    test_reset_and_high_water_mark();

    printf("=== All Arena Tests Passed! ===\n");
    return 0;
}
