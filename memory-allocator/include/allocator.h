#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

/* Simplified malloc/free/calloc/realloc, built directly on top of the
 * sbrk() syscall wrapper (no libc malloc involved). See README.md for
 * the design and for how this fits into the OS memory stack. */

void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_calloc(size_t nmemb, size_t size);
void *my_realloc(void *ptr, size_t size);

/* Debug helper: dumps the block list to stderr (address, size, free/used). */
void my_heap_dump(void);

#endif /* ALLOCATOR_H */
