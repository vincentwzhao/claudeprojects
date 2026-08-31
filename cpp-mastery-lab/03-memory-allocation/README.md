# 03 — Memory allocation

## The concept

Every object needs storage, and that storage comes from one of three places:

1. **Static storage** — globals and `static` locals. Allocated once, lives
   for the whole program.
2. **Automatic (stack) storage** — local variables. Allocated on function
   entry, freed on function exit, essentially free (a stack pointer bump).
3. **Dynamic (heap/free store) storage** — explicitly requested with `new` /
   `malloc`, explicitly released with `delete` / `free`. Lives until you say
   otherwise, which is exactly the danger.

## `new`/`delete` vs `malloc`/`free`

| | `malloc`/`free` | `new`/`delete` |
|---|---|---|
| Runs constructors/destructors | no | yes |
| Type-aware | no (`void*`) | yes |
| Failure signal | returns `NULL` | throws `std::bad_alloc` |
| Array form | manual size math | `new T[n]` / `delete[]` |

**Never mix them.** `free`ing a `new`'d pointer (or vice versa) is undefined
behavior — they may use different bookkeeping. And `delete` vs `delete[]`
mismatch is its own UB: `delete[]` needs to know the array length that
`new[]` stashed; using plain `delete` on an array skips reading it correctly.

## What "leak" actually means

A leak is memory you allocated and lost the last pointer to, so nothing can
ever `delete`/`free` it. The process keeps that memory until it exits. Small
one-shot leaks in short programs barely matter; leaks inside a loop or a
long-running server exhaust memory and eventually crash or get OOM-killed.

## Alignment, in brief

Objects must start at addresses that are multiples of their alignment
requirement (`alignof(T)`) — the CPU (or at least the compiler-generated
code) expects it. `new`/`malloc` return memory aligned for any built-in
type; `alignas` lets you request stricter alignment (relevant for SIMD types
or cache-line padding, see `04-stack-vs-heap`).

## Modern C++: prefer not calling `new`/`delete` directly

`06-templates`/`07-stl`/`08-raii` cover this properly, but the headline: in
real code you almost always want `std::make_unique`/`std::make_shared` or a
container, not raw `new`. This module exists so you understand what those
wrappers are doing underneath, not so you write raw `new`/`delete` in
production.

## Common traps

- Double free / double delete.
- Use-after-free (dereferencing a pointer after its memory was released).
- Mismatched `new`/`free` or `new[]`/`delete`.
- Forgetting `delete[]` for array `new[]`.
- Leaking on early-return / exception paths — this is *the* motivating
  problem for RAII (`08-raii`).

## Run it

```bash
./03-memory-allocation-demo
./03-memory-allocation-exercises
# Build with -DSANITIZE=ON from the repo root to catch leaks/UAF for real.
```
