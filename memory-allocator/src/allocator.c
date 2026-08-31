/*
 * A simplified malloc()/free() implementation.
 *
 * Design: implicit, address-ordered doubly linked list of blocks living
 * inside a single contiguous heap region obtained from the kernel with
 * sbrk(). Each allocation is prefixed by a small header; first-fit search
 * finds a free block, splitting it if there's enough left over to be
 * useful. free() marks a block free and coalesces it with its immediate
 * neighbors; if the freed block is now the last block in the heap, the
 * memory is handed back to the kernel with sbrk(-n).
 *
 * This is an educational allocator: single heap, first-fit, thread-safe
 * via one global lock (no per-thread arenas), no mmap for large
 * allocations. Real allocators (ptmalloc/glibc, jemalloc, tcmalloc) add
 * segregated free lists / bins by size class, per-thread arenas, and
 * mmap() for large requests to avoid this allocator's two weak points:
 * O(n) first-fit search and heap fragmentation.
 */

#define _DEFAULT_SOURCE /* expose sbrk() under -std=c11 */

#include "allocator.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~((size_t)ALIGNMENT - 1))

typedef struct block_header {
    size_t size;              /* usable size, in bytes, excluding this header */
    int free;                 /* 1 if this block is available for reuse */
    struct block_header *next; /* next block by ascending address, or NULL */
    struct block_header *prev; /* previous block by ascending address, or NULL */
} block_header_t;

#define HEADER_SIZE ALIGN(sizeof(block_header_t))
/* Only split a block if the leftover chunk can itself hold a header plus
 * at least one alignment quantum of payload; otherwise the leftover slice
 * would be too small to ever be allocated and we'd just be wasting a
 * header's worth of bookkeeping on it. */
#define MIN_SPLIT_REMAINDER (HEADER_SIZE + ALIGNMENT)

static block_header_t *g_head = NULL; /* first block in the heap, by address */
static block_header_t *g_tail = NULL; /* last block in the heap, by address */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static block_header_t *header_of(void *ptr) {
    return ((block_header_t *)ptr) - 1;
}

static void *payload_of(block_header_t *block) {
    return (void *)(block + 1);
}

/* Grows the heap by exactly enough to hold a header + `size` bytes of
 * payload, and returns the new block (unlinked; caller wires it in). */
static block_header_t *grow_heap(size_t size) {
    void *raw = sbrk((intptr_t)(HEADER_SIZE + size));
    if (raw == (void *)-1) {
        return NULL; /* kernel refused to extend the break, e.g. OOM */
    }
    block_header_t *block = (block_header_t *)raw;
    block->size = size;
    block->free = 0;
    block->next = NULL;
    block->prev = NULL;
    return block;
}

static block_header_t *find_first_fit(size_t size) {
    for (block_header_t *b = g_head; b != NULL; b = b->next) {
        if (b->free && b->size >= size) {
            return b;
        }
    }
    return NULL;
}

/* If `block` has enough spare room beyond `size`, carve a new free block
 * out of the tail end of it and splice it into the list right after. */
static void split_block(block_header_t *block, size_t size) {
    size_t remainder = block->size - size;
    if (remainder < MIN_SPLIT_REMAINDER) {
        return; /* not worth splitting */
    }

    block_header_t *new_block =
        (block_header_t *)((char *)payload_of(block) + size);
    new_block->size = remainder - HEADER_SIZE;
    new_block->free = 1;
    new_block->prev = block;
    new_block->next = block->next;

    if (block->next) {
        block->next->prev = new_block;
    } else {
        g_tail = new_block;
    }
    block->next = new_block;
    block->size = size;
}

/* Merges `block` with its next neighbor if that neighbor is free and
 * physically adjacent (it always is, in an address-ordered list built by
 * a single contiguous sbrk-backed heap). */
static void coalesce_with_next(block_header_t *block) {
    block_header_t *next = block->next;
    if (!next || !next->free) {
        return;
    }
    block->size += HEADER_SIZE + next->size;
    block->next = next->next;
    if (next->next) {
        next->next->prev = block;
    } else {
        g_tail = block;
    }
}

/* If the last block in the heap is free, shrink the break so the OS gets
 * the memory back instead of it sitting idle at the end of our arena. */
static void release_tail_if_free(void) {
    if (!g_tail || !g_tail->free) {
        return;
    }
    block_header_t *dead = g_tail;
    g_tail = dead->prev;
    if (g_tail) {
        g_tail->next = NULL;
    } else {
        g_head = NULL;
    }
    sbrk(-(intptr_t)(HEADER_SIZE + dead->size));
}

void *my_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    size = ALIGN(size);

    pthread_mutex_lock(&g_lock);

    block_header_t *block = find_first_fit(size);
    if (block) {
        block->free = 0;
        split_block(block, size);
    } else {
        block = grow_heap(size);
        if (!block) {
            pthread_mutex_unlock(&g_lock);
            return NULL;
        }
        block->prev = g_tail;
        if (g_tail) {
            g_tail->next = block;
        } else {
            g_head = block;
        }
        g_tail = block;
    }

    pthread_mutex_unlock(&g_lock);
    return payload_of(block);
}

void my_free(void *ptr) {
    if (!ptr) {
        return;
    }

    pthread_mutex_lock(&g_lock);

    block_header_t *block = header_of(ptr);
    block->free = 1;

    /* Coalesce forward first so that a subsequent backward coalesce (by
     * merging *into* prev) absorbs the already-merged, larger block. */
    coalesce_with_next(block);
    if (block->prev && block->prev->free) {
        block = block->prev;
        coalesce_with_next(block);
    }

    release_tail_if_free();

    pthread_mutex_unlock(&g_lock);
}

void *my_calloc(size_t nmemb, size_t size) {
    if (nmemb != 0 && size > (size_t)-1 / nmemb) {
        return NULL; /* would overflow nmemb * size */
    }
    size_t total = nmemb * size;
    void *ptr = my_malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void *my_realloc(void *ptr, size_t size) {
    if (!ptr) {
        return my_malloc(size);
    }
    if (size == 0) {
        my_free(ptr);
        return NULL;
    }

    block_header_t *block = header_of(ptr);
    if (block->size >= size) {
        return ptr; /* current block is already big enough */
    }

    void *new_ptr = my_malloc(size);
    if (!new_ptr) {
        return NULL;
    }
    memcpy(new_ptr, ptr, block->size);
    my_free(ptr);
    return new_ptr;
}

void my_heap_dump(void) {
    pthread_mutex_lock(&g_lock);
    fprintf(stderr, "---- heap dump ----\n");
    size_t i = 0;
    for (block_header_t *b = g_head; b != NULL; b = b->next, i++) {
        fprintf(stderr, "  [%zu] addr=%p size=%zu %s\n", i, (void *)b,
                b->size, b->free ? "FREE" : "USED");
    }
    fprintf(stderr, "-------------------\n");
    pthread_mutex_unlock(&g_lock);
}
