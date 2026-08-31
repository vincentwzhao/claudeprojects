# Design notes

This is the document to walk an interviewer through — it explains the
mechanisms, not just what the code does.

## Why three allocators, not one

Each one trades away a capability the others have, in exchange for speed:

| | Individual free? | Variable size? | Alloc cost | Free cost |
|---|---|---|---|---|
| `BumpArena` | No (whole-arena `reset()` only) | Yes | O(1) | N/A |
| `PoolAllocator<T>` | Yes | No (fixed size = `sizeof(T)`) | O(1) | O(1) |
| `FreeListAllocator` | Yes | Yes | O(free blocks), first-fit | O(1) + coalesce |

`malloc`/`new` sit at the "yes to everything, but slower per-op and with
allocator-metadata overhead" end of this tradeoff space, because they have
to stay correct for every possible usage pattern simultaneously. Each
allocator here is fast specifically because it gives up guarantees a
general-purpose allocator can't give up.

## Alignment (`include/align.hpp`)

Every allocator needs to hand back memory aligned to at least
`alignof(std::max_align_t)` (or a caller-specified stricter alignment).
The trick, used identically in all three allocators:

```cpp
constexpr std::uintptr_t align_up(std::uintptr_t addr, std::size_t alignment) {
    return (addr + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
}
```

This only works because every valid alignment in C++ is a power of two
(`alignof(T)` is guaranteed to be). For a power-of-two `a`, `~(a-1)` is a
mask with all the low bits that must be zero for an `a`-aligned address —
ANDing with it after adding `a-1` rounds up without a division or modulo,
which matters here since this runs on every single allocation.

## `BumpArena`: the tradeoff is explicit, not hidden

`reset()` does not call destructors — it just rewinds an offset to 0. This
is stated as a design decision in the header, not glossed over: an arena
that tracked per-object destructors to call on reset would need to store a
destructor thunk per allocation, which destroys the entire reason to use
an arena (O(1) bulk deallocation with zero bookkeeping). Real arena
allocators (protobuf's `Arena`, most game-engine frame allocators) make
the same tradeoff for the same reason — it's meant for POD-ish, batch-
lifetime data, not RAII types you need destroyed individually.

## `PoolAllocator<T>`: the intrusive free list

The free list costs **zero extra memory** because it's stored inside the
blocks it's tracking — specifically, inside the blocks that are currently
free (nothing else is using that memory right now, so borrowing its first
`sizeof(void*)` bytes for a `next` pointer is safe):

```
[ FreeNode{next} | ...unused... ]  <- a free block, block_size bytes
[ FreeNode{next} | ...unused... ]  <- another free block
```

The moment a block is allocated, those bytes become the caller's object
storage instead — the free-list pointer that used to live there is simply
overwritten, which is fine because a block only needs a `next` pointer
while it's *on* the free list.

## `FreeListAllocator`: boundary tags (the K&R `malloc` technique)

Each block — free or allocated — carries a `Header` (size + free flag) at
its start and a `Footer` (size only) at its end:

```
[Header|size,free][ ...payload... ][Footer|size][Header|size,free][ ...payload... ][Footer|size]
```

**Splitting**: when a free block is bigger than the request, carve off the
front `needed` bytes as the allocated block and leave the remainder as a
new (smaller) free block right after it — but only if the remainder is
large enough to be a useful block on its own (`>= header + footer +
min payload`). Otherwise, hand over the whole block; a sliver too small to
ever satisfy a future request is worse than a little internal
fragmentation.

**Coalescing** (the interesting part): on `deallocate`, check both physical
neighbors —

- **Next neighbor**: trivial. It starts right after this block ends;
  read its header, and if it's free, merge.
- **Previous neighbor**: this is what the *footer* is for. You can't walk
  backward through a singly-forward block layout without it — the footer
  sitting immediately before this block's header tells you that previous
  block's size, and therefore where *its* header starts, in O(1), with no
  global block index needed.

Without coalescing, a churn pattern of alloc/free at varying sizes would
fragment the arena into unusably small free slivers over time. The stress
test (`tests/test_stress.cpp`) exercises exactly this pattern under
AddressSanitizer to make sure splitting and coalescing never corrupt a
neighboring live block — a heap-buffer-overflow in this code would almost
certainly show up as ASan flagging a `write` into what should have been
another block's payload.

## `StlAllocator<T, Backend>`: satisfying the Allocator named requirements

To be usable as `std::vector<T, StlAllocator<T, Backend>>`, a type needs
(at minimum): a `value_type` member, `allocate(n)`/`deallocate(p, n)`, a
converting constructor from `StlAllocator<U, Backend>` (so the container
can rebind to allocate its own internal types), and `operator==`/`!=` (so
the container can tell whether two allocator instances are interchangeable
— they are here iff they point at the same `Backend`).

It's deliberately a *non-owning view* of a `Backend` the caller constructs
and keeps alive, not an owner — the same design `std::pmr` allocators use.
`PoolAllocator` is **not** wrapped this way: a container asking for `n`
contiguous `T`s (as `vector` does on every growth) can't be satisfied by a
pool that only hands out one fixed-size, non-contiguous block at a time.
Pairing a pool with something like `std::list` (which only ever needs one
node at a time) would work, but requires the pool's block size to match
that container's internal node type exactly, which is finicky enough to
be a stretch goal rather than in scope here (see below).

## Thread-safety: explicitly out of scope for v1

None of these allocators are safe to call concurrently from multiple
threads — no locks, no atomics. This was a scoping decision, not an
oversight: making `PoolAllocator`'s free list thread-safe with a
`std::mutex` is a small, obvious change; making it thread-safe *without* a
lock (a lock-free stack via CAS, avoiding the ABA problem) is a
substantially harder, separate project that deserves its own focused time
rather than being bolted on. See `10-concurrency` in the companion
`cpp-mastery-lab` repo for the primitives this would build on.

## Honest benchmark results, including a loss

See the root `README.md` for full numbers. The short version: `PoolAllocator`
beats `malloc`/`new` by roughly 30-50x on same-size churn, `BumpArena`
beats individual `malloc`+`free` by ~5-6x on the batch-reset pattern, and
the pool's contiguous layout gives a measurable (~2x) linked-list traversal
speedup from cache locality — but `FreeListAllocator` is *slower* than
glibc's `malloc` on the variable-size live-working-set workload. That's
expected, not a bug: glibc's allocator uses segregated free lists (bins per
size class) for near-O(1) average-case behavior, while this one does a
linear first-fit scan over all free blocks. Reporting that loss honestly
is more useful — and more credible — than only running the benchmarks that
flatter the code.

## Stretch goals (deliberately not built, roadmap only)

- Segregated free lists (size-class bins) in `FreeListAllocator`, to close
  the gap with `malloc` on workload 2.
- Thread-safe variants (mutex-based first, lock-free stack for the pool as
  a follow-up).
- A slab allocator: multiple `PoolAllocator`-style pools bucketed by size
  class, so "fixed size only" stops being a real limitation for common
  small-object workloads.
- Guard bytes / redzones around each allocation (a tiny hand-rolled ASan)
  to catch small overflows even in a non-sanitized build.
- Global `operator new`/`operator delete` override to point a whole
  program's default allocations at one of these, and re-run an existing
  benchmark suite (e.g. a JSON parser or the linked-list workload at a
  much larger scale) against it end-to-end.
