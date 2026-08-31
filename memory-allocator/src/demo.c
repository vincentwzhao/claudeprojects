/* Small interactive-ish demo showing the allocator in action and how the
 * process break (heap end) moves as we allocate, free, and coalesce. */

#define _DEFAULT_SOURCE /* expose sbrk() under -std=c11 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "allocator.h"

static void print_break(const char *label) {
    printf("%-28s program break = %p\n", label, sbrk(0));
}

int main(void) {
    print_break("start");

    char *a = my_malloc(64);
    strcpy(a, "hello from block a");
    print_break("after malloc(64) -> a");

    char *b = my_malloc(128);
    strcpy(b, "hello from block b");
    print_break("after malloc(128) -> b");

    char *c = my_malloc(32);
    strcpy(c, "hello from block c");
    print_break("after malloc(32) -> c");

    printf("a: %s\n", a);
    printf("b: %s\n", b);
    printf("c: %s\n", c);
    my_heap_dump();

    my_free(b);
    printf("freed b (middle block, stays in free list for reuse)\n");
    my_heap_dump();

    char *d = my_malloc(100);
    strcpy(d, "reused b's slot");
    printf("d: %s\n", d);
    print_break("after malloc(100) -> d (reuses freed b)");
    my_heap_dump();

    my_free(a);
    my_free(c);
    my_free(d);
    print_break("after freeing everything (heap shrinks back)");
    my_heap_dump();

    return 0;
}
