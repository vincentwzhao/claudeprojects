# 04 — Stack vs. heap

## The concept

Two very different allocators, and knowing which one you're using explains
most "why is this slow" and "why did this crash" questions.

### Stack

- A contiguous region per thread, fixed size (often 1–8 MB, `ulimit -s` on
  Linux).
- Allocation = decrementing a pointer; deallocation = incrementing it back.
  No bookkeeping, no fragmentation — this is why local variables are cheap.
- LIFO by construction: the most recently pushed frame is always the first
  popped. This is exactly a function call stack — each call pushes a frame,
  each return pops it.
- Fixed size at compile time per frame (variable-length arrays aside) — you
  can't stack-allocate something whose size is only known at runtime in
  standard C++ (that's what the heap is for).
- Overflow it (too much recursion, a huge local array) and you get a stack
  overflow — typically a hard crash (SIGSEGV), not a catchable exception.

### Heap (free store)

- A large shared region managed by an allocator (glibc's `malloc`, or
  `new`'s underlying allocator) that tracks free/used blocks.
- Allocation requires the allocator to search for/carve out a suitably
  sized free block — real work, so heap allocation is measurably slower
  than a stack push.
- Lifetime is manual (or RAII-managed) — it doesn't end when a function
  returns, which is exactly why it's useful for data that must outlive its
  creating scope.
- Can fragment: many alloc/free cycles of varying sizes can leave the heap
  full of small unusable gaps.

## The concrete comparison

| | Stack | Heap |
|---|---|---|
| Speed | very fast (pointer bump) | slower (allocator bookkeeping) |
| Lifetime | scope-bound, automatic | manual / RAII-controlled |
| Size limit | small, fixed per thread | large, limited by RAM/address space |
| Fragmentation | none | possible |
| Failure mode | stack overflow (crash) | allocation failure (`bad_alloc`/null) |

## Why this matters for computer architecture too

Stack memory for a hot function is very likely to already be in L1 cache
(you just touched it), which is part of why "avoid unnecessary heap
allocation in hot loops" is real advice, not superstition — it's not just
the allocator overhead, it's cache locality.

## Common traps

- Returning a pointer/reference to a stack-local (dangling — see `01` and
  `02`).
- Deep recursion or huge local arrays overflowing the stack.
- Assuming heap allocation is "free" and doing it in a hot loop.
- Believing "stack" and "value type" / "heap" and "pointer type" are the
  same axis — they're not: `std::vector<int> v;` is a stack *object* whose
  internal buffer lives on the heap. Where the object's own storage lives
  and where the data it manages lives can differ.

## Run it

```bash
./04-stack-vs-heap-demo
./04-stack-vs-heap-exercises
```
