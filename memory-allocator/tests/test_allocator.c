#define _DEFAULT_SOURCE /* expose sbrk() under -std=c11 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "allocator.h"

static int tests_run = 0;

#define RUN(fn)                                                              \
    do {                                                                     \
        printf("running %-32s ... ", #fn);                                  \
        fflush(stdout);                                                      \
        fn();                                                                \
        tests_run++;                                                         \
        printf("ok\n");                                                      \
    } while (0)

static void test_malloc_returns_writable_memory(void) {
    char *p = my_malloc(32);
    assert(p != NULL);
    memset(p, 'x', 32);
    assert(p[0] == 'x' && p[31] == 'x');
    my_free(p);
}

static void test_malloc_zero_returns_null(void) {
    assert(my_malloc(0) == NULL);
}

static void test_free_null_is_noop(void) {
    my_free(NULL); /* must not crash */
}

static void test_blocks_do_not_overlap(void) {
    char *a = my_malloc(16);
    char *b = my_malloc(16);
    char *c = my_malloc(16);
    memset(a, 'a', 16);
    memset(b, 'b', 16);
    memset(c, 'c', 16);
    for (int i = 0; i < 16; i++) {
        assert(a[i] == 'a');
        assert(b[i] == 'b');
        assert(c[i] == 'c');
    }
    my_free(a);
    my_free(b);
    my_free(c);
}

static void test_free_and_reuse(void) {
    void *p1 = my_malloc(64);
    my_free(p1);
    void *p2 = my_malloc(64);
    /* First-fit + address-ordered list means the freed block should be
     * handed straight back out rather than growing the heap again. */
    assert(p1 == p2);
    my_free(p2);
}

static void test_coalesce_adjacent_free_blocks(void) {
    void *a = my_malloc(32);
    void *b = my_malloc(32);
    void *c = my_malloc(32);
    (void)c;

    my_free(a);
    my_free(b);
    /* a and b are now one merged free block; a request that would not
     * have fit in either alone should fit in the coalesced block. */
    void *big = my_malloc(64 + 16 /* header slack from ALIGN */);
    assert(big == a);

    my_free(big);
    my_free(c);
}

static void test_heap_shrinks_when_tail_freed(void) {
    void *before = sbrk(0);
    void *p = my_malloc(4096);
    void *grown = sbrk(0);
    assert(grown != before);
    my_free(p);
    void *after = sbrk(0);
    assert(after == before);
}

static void test_calloc_zeroes_memory(void) {
    unsigned char *p = my_calloc(10, sizeof(unsigned char));
    for (int i = 0; i < 10; i++) {
        assert(p[i] == 0);
    }
    my_free(p);
}

static void test_realloc_grow_preserves_data(void) {
    char *p = my_malloc(8);
    memcpy(p, "abcdefg", 8);
    char *q = my_realloc(p, 64);
    assert(memcmp(q, "abcdefg", 8) == 0);
    my_free(q);
}

static void test_realloc_shrink_keeps_pointer(void) {
    char *p = my_malloc(64);
    memcpy(p, "hello", 6);
    char *q = my_realloc(p, 8);
    assert(q == p);
    assert(memcmp(q, "hello", 6) == 0);
    my_free(q);
}

static void test_realloc_null_acts_like_malloc(void) {
    char *p = my_realloc(NULL, 16);
    assert(p != NULL);
    my_free(p);
}

static void test_realloc_zero_size_frees(void) {
    char *p = my_malloc(16);
    assert(my_realloc(p, 0) == NULL);
}

static void test_many_small_allocations_stress(void) {
    enum { N = 2000 };
    void *ptrs[N];
    for (int i = 0; i < N; i++) {
        ptrs[i] = my_malloc(16 + (i % 32));
        assert(ptrs[i] != NULL);
        memset(ptrs[i], (unsigned char)i, 16 + (i % 32));
    }
    for (int i = 0; i < N; i++) {
        my_free(ptrs[i]);
    }
    /* Everything freed in address order (roughly), heap should return to
     * (near) its starting size thanks to tail release + coalescing. */
    void *p = my_malloc(16);
    my_free(p);
}

int main(void) {
    RUN(test_malloc_returns_writable_memory);
    RUN(test_malloc_zero_returns_null);
    RUN(test_free_null_is_noop);
    RUN(test_blocks_do_not_overlap);
    RUN(test_free_and_reuse);
    RUN(test_coalesce_adjacent_free_blocks);
    RUN(test_heap_shrinks_when_tail_freed);
    RUN(test_calloc_zeroes_memory);
    RUN(test_realloc_grow_preserves_data);
    RUN(test_realloc_shrink_keeps_pointer);
    RUN(test_realloc_null_acts_like_malloc);
    RUN(test_realloc_zero_size_frees);
    RUN(test_many_small_allocations_stress);

    printf("\nAll %d tests passed.\n", tests_run);
    return 0;
}
