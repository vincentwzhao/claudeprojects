# cpp-allocators

Three memory allocators written from scratch — a bump arena, a fixed-size
pool, and a general-purpose free-list allocator — plus an adapter that
makes any of them a drop-in `std::vector`/STL allocator, and a benchmark
suite comparing all of them against `malloc`/`new` with real, measured
numbers.

This exists to answer, concretely, "do you actually understand memory
allocation, or do you just call `new` and hope": pointers, alignment,
stack vs. heap, RAII, templates, and the STL allocator model all show up
here because the problem genuinely needs them, not because it's a
checklist. See [`docs/DESIGN.md`](docs/DESIGN.md) for how each one works
internally — that's the document to walk through in an interview.

## What's here

| | What it does | Best for |
|---|---|---|
| `BumpArena` | O(1) pointer-bump allocation; frees everything at once via `reset()` | Batch/scoped allocations that all die together |
| `PoolAllocator<T>` | O(1) alloc *and* free via an intrusive free list, fixed block size | Heavy churn of same-sized objects (nodes, particles, connections) |
| `FreeListAllocator` | Variable-size allocate/free over one buffer, with splitting + coalescing | General-purpose use where sizes vary and lifetimes are independent |
| `StlAllocator<T, Backend>` | Wraps `BumpArena` or `FreeListAllocator` to satisfy the C++ `Allocator` requirements | `std::vector<T, StlAllocator<T, FreeListAllocator>>` and friends |

## Build & run

```bash
mkdir build && cd build
cmake ..
make -j
ctest --output-on-failure     # unit tests for all three allocators + the STL adapter
./benchmarks/bench_main        # real throughput numbers, see below
```

### Sanitizer build (recommended while reading/modifying the allocators)

```bash
cmake -DSANITIZE=ON ..
make -j
ctest --output-on-failure
./tests/test_stress             # randomized alloc/free fuzzing under ASan+UBSan
```

`tests/test_stress.cpp` runs 20,000+ randomized alloc/free/write cycles
against each allocator with a byte-pattern check on every live allocation
— any heap overflow, use-after-free, or corrupted neighbor from a bad
split/coalesce shows up immediately as an ASan report with a stack trace.

## Benchmark results

Measured on this machine (Intel Xeon @ 2.10GHz, GCC 13.3.0, `-O2`,
single run, no sanitizers) — reproduce with `./benchmarks/bench_main`,
numbers will vary by machine and by run:

**[1] Fixed-size (32B) alloc+free churn, 2,000,000 iterations**
| Allocator | Time | Throughput |
|---|---|---|
| `malloc`/`free` | 16.6 ms | 121M ops/sec |
| `new`/`delete` | 23.9 ms | 84M ops/sec |
| `PoolAllocator` | **0.6 ms** | **3.4B ops/sec** |
| `FreeListAllocator` | 14.5 ms | 138M ops/sec |

Pool wins big here — ~30-40x faster than `malloc`. This is the workload a
pool allocator exists for: no search, no bookkeeping beyond one pointer
swap per alloc/free.

**[2] Variable-size (16-256B) alloc/free with a live working set, 300,000 ops**
| Allocator | Time | Throughput |
|---|---|---|
| `malloc`/`free` | 6.3 ms | 47M ops/sec |
| `FreeListAllocator` | 9.6 ms | 31M ops/sec |

`FreeListAllocator` **loses** to glibc's `malloc` here, by about 1.5x —
and that's the honest, expected result, not a bug. glibc uses segregated
free lists (bins per size class) for near-O(1) average allocation; this
implementation does a linear first-fit scan over free blocks. Closing that
gap (segregated bins) is a stated stretch goal in `DESIGN.md`, not
something papered over.

**[3] Batch-allocate-then-free-all-at-once, 200 batches × 5,000 objects**
| Allocator | Time | Throughput |
|---|---|---|
| `malloc` + individual `free` | 10.8 ms | 93M ops/sec |
| `BumpArena` + `reset()` | **1.7-2.2 ms** | **~500M ops/sec** |

Arena wins ~5-6x by skipping per-object free work entirely — one
`reset()` reclaims the whole batch.

**[4] Build + traverse a 1,000,000-node linked list**
| Allocator | Build time | Traverse time |
|---|---|---|
| `new` (scattered) | 25.0 ms | 5.0 ms |
| `PoolAllocator` (contiguous) | **2.5 ms** | **2.6 ms** |

Pool wins ~10x on build (no per-node `malloc` overhead) and ~2x on
traversal — the traversal number is the interesting one, since it's not
measuring allocation speed at all, it's measuring cache locality: the
pool's nodes sit in one contiguous, strided region of memory, so walking
the list is far more cache-friendly than chasing pointers scattered across
the heap by individual `new` calls.

## What I'd add next

See `docs/DESIGN.md`'s "Stretch goals" section — segregated free lists to
close the gap in workload 2, thread-safe variants, a slab allocator, and
guard-byte overflow detection are the next things on the list, in that
order.
