# Memory Allocator

A simplified `malloc()` / `free()` implementation in C, built directly on
top of `sbrk()` (no libc allocator involved), plus a walkthrough of what
actually happens underneath a call to `malloc`.

## Layout

```
memory-allocator/
├── include/allocator.h     public API: my_malloc, my_free, my_calloc, my_realloc
├── src/allocator.c         the allocator itself
├── src/demo.c              small program that visualizes the heap as it's used
├── tests/test_allocator.c  assert-based test suite
└── Makefile
```

Build and run:

```sh
make        # builds bin/demo and bin/test_allocator, runs the tests
./bin/demo  # watch the heap grow, get reused, and shrink
```

## How the allocator works

**Data structure.** The heap is one address-ordered, doubly linked list of
blocks. Each block is a small header immediately followed by the payload
handed back to the caller:

```
 ┌──────────────┬─────────────────────────────┐
 │ block_header │           payload            │
 │ size|free|*n |*p                            │
 └──────────────┴─────────────────────────────┘
 ^                ^
 header           pointer returned by my_malloc()
```

`my_free(ptr)` recovers the header with `((block_header_t *)ptr) - 1` —
this is why every allocator needs *some* out-of-band bookkeeping: the
caller only ever sees the payload pointer, never the size.

**Allocation (`my_malloc`).**
1. Round the request up to a 16-byte multiple (alignment matters — see
   below).
2. Walk the block list looking for the first free block big enough
   (*first-fit*).
3. If found: mark it used, and if it has more room than needed, split off
   the leftover as a new free block so it isn't wasted.
4. If nothing fits: ask the kernel for more memory with `sbrk()` and
   append a brand-new block at the end of the heap.

**Freeing (`my_free`).**
1. Mark the block free.
2. Coalesce with the next block if it's also free, and with the previous
   block if *that's* free — this is what stops the heap from fragmenting
   into a graveyard of tiny unusable free blocks.
3. If the now-free block is the last one in the heap, shrink the break
   with `sbrk(-n)` and actually give the memory back to the kernel.

**Why first-fit + address-ordered coalescing?** It's the simplest
correct design and it's enough to make the fragmentation and reuse
behavior visible (see `demo.c`). Production allocators (glibc's
ptmalloc, jemalloc, tcmalloc) instead keep several free lists segregated
by size class ("bins") so allocation is close to O(1) instead of the
O(n) list walk here, add a per-thread arena so threads don't fight over
one global lock, and route large requests to `mmap()` instead of `sbrk()`
so a single huge allocation can be released back to the OS immediately
rather than only when it happens to be at the end of the heap.

**Why 16-byte alignment?** SSE/AVX instructions and `double`/`long`
values require aligned addresses on most ABIs; the C standard requires
`malloc` to return memory suitably aligned for *any* object, and 16 bytes
is the largest fundamental alignment on x86-64/AArch64.

## The bigger picture: `application → malloc → heap → virtual memory → physical memory`

```
 process address space                          physical RAM
 ┌─────────────────────────┐
 │ stack (grows down)       │
 │        ...                │
 │ mmap'd libraries/files    │
 │        ...                │
 │ HEAP  ─┐                  │        page tables (per-process)
 │  used  │  ← this project  │        ┌───────────────────────┐
 │  free  │  lives entirely  │  MMU   │ virtual page → frame,  │
 │  used  │  inside here     │ ─────► │ or "not present"       │
 │        │                  │        └───────────────────────┘
 │ "brk" ─┘ ← program break  │                 │
 │        (unmapped)         │                 ▼
 ├─────────────────────────┤          physical page frames
 │ .bss / .data / .text     │          (RAM, shared/managed by
 └─────────────────────────┘           the kernel across all processes)
```

1. **Application calls `malloc(n)`.** This is a pure userspace library
   call — no syscall in the common case. The allocator (this project's
   `my_malloc`, or glibc's `malloc` in real life) manages a pool of
   memory it already owns and just hands out a chunk of it. That pool is
   the **heap**.

2. **The heap is a region of the process's virtual address space** that
   the allocator grows on demand. It grows the heap by asking the kernel
   to move the **program break** (`sbrk()`/`brk()`) — or, for large
   allocations, by asking for an anonymous mapping with `mmap()`. Either
   way this *is* a syscall, but it only happens occasionally (when the
   allocator runs out of free blocks to reuse), not on every `malloc`
   call — which is the whole point of having a userspace allocator sit in
   front of the kernel.

3. **Virtual memory**: what `sbrk()` actually grew is a range of
   *virtual* addresses in this process's page tables — not real memory.
   The kernel just promises "this range belongs to you," typically
   without allocating physical RAM yet. Virtual memory is what gives
   every process its own private, contiguous-looking address space and
   is what makes memory isolation between processes possible: two
   processes can both believe they own address `0x55a12a385000`, and the
   MMU (memory management unit) makes sure each one is actually looking
   at different physical RAM.

4. **Physical memory**: RAM is only actually committed when the CPU first
   touches a virtual page inside that new range and the MMU finds no
   mapping for it — a **page fault**. The kernel's fault handler picks a
   free physical page frame, records the virtual→physical mapping in the
   page table, and resumes the instruction, now transparently. This is
   why `sbrk()`/`mmap()` can be "cheap" even for a large request: no RAM
   is actually touched until pages are used, and pages that are asked for
   but never written may never cost real memory at all (this is also
   how `demo.c`'s `malloc(4096)` can look instant even though it grew the
   virtual heap by a full page).

5. When `free()` gives memory back to the allocator, nothing changes at
   the OS level — the block just re-enters the free list for the *next*
   `malloc()` call to reuse, which is far cheaper than another syscall.
   Only when a whole run of memory at the *end* of the heap becomes free
   does this allocator call `sbrk()` with a negative size, actually
   shrinking the process's virtual address space and letting the kernel
   reclaim the backing physical pages.

So the chain in the prompt reads, in practice:

```
application: my_malloc(100)
   → userspace allocator: found/carved a free block, no syscall needed
       (only on the rare "heap is out of space" path) ↓
   → sbrk(n): extend the heap, i.e. extend this process's virtual
       address space — a syscall, but an infrequent one
   → virtual memory: kernel updates this process's page table to mark
       the new range valid-but-unbacked
   → physical memory: RAM is committed lazily, one 4 KiB page at a time,
       the first time each page is actually touched (page fault)
```
