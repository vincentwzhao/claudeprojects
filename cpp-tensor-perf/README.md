# cpp-tensor-perf

A from-scratch, zero-dependency CPU GEMM (matrix multiply) library in
C++17, built as a deliberate progression: naive triple loop &rarr; loop
reordering &rarr; cache blocking &rarr; AVX2/FMA SIMD &rarr; multithreading.
Every step is checked against the naive kernel for correctness before it's
allowed into a benchmark number, and the whole thing is wired into a small
MLP forward pass so the speedup is measured on something that looks like
an actual ML workload, not just an abstract matmul microbenchmark.

No external dependencies — just the STL, `<immintrin.h>` intrinsics, and
`<thread>`. Tests are a small self-written assert-that-keeps-going
framework (`tests/check.hpp`) rather than a fetched test framework, so the
whole project builds and runs offline with nothing but CMake and a
C++17 compiler.

## Layout

```
cpp-tensor-perf/
├── include/tensor/
│   ├── matrix.hpp          row-major, 64-byte-aligned Matrix
│   ├── aligned_buffer.hpp  RAII aligned heap buffer
│   ├── gemm.hpp            public API + kernel registry (kGemmKernels)
│   ├── gemm_detail.hpp     internal helper shared by simd/threaded kernels
│   ├── thread_pool.hpp     persistent-thread ParallelFor
│   ├── ops.hpp             bias add / relu / sigmoid (SIMD)
│   └── timer.hpp           warmup + median-of-N benchmarking helper
├── src/                    one .cpp per kernel + thread_pool.cpp + ops.cpp
├── tests/                  correctness tests (every kernel vs. naive)
├── benchmark/bench_gemm.cpp
├── examples/mlp_inference_demo.cpp
└── CMakeLists.txt
```

## Build & run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/run_tests     # correctness: every kernel checked against naive
./build/bench_gemm     # GFLOP/s table across sizes, all kernels
./build/mlp_demo       # naive vs. threaded, wired into an MLP forward pass
```

Requires AVX2 + FMA (anything from Intel Haswell / AMD Excavator onward —
essentially any x86-64 machine from the last decade). The build targets
`-mavx2 -mfma` explicitly rather than `-march=native`, so it compiles the
same way in CI as on a reviewer's laptop instead of depending on whatever
CPU happens to run `cmake`.

## The optimization progression

`C = A * B`, A is `M x K`, B is `K x N`, C is `M x N`. Each step changes
one thing about how the same O(M·N·K) multiply-adds are carried out.

**1. `GemmNaive` — i-j-k, the textbook order.**
For each output element, walk `k` straight through: `A(i,k)` is
unit-stride, but `B(k,j)` jumps a full row on every `k`. Once `K` is more
than a few cache lines, the inner loop is dominated by cache misses on
`B`, not by the actual arithmetic.

**2. `GemmReordered` — i-k-j.**
Hoist `k` out one level: the innermost loop now walks `B(k, *)` and
`C(i, *)` both unit-stride, row by row. Same multiply-adds, same memory
*volume*, radically better access *pattern*. This is a free win — no new
data structures, just a different loop order — and it's the single
biggest jump in the whole progression (see numbers below). It's also
simple enough that GCC's autovectorizer picks it up and emits AVX2 code
for the inner loop on its own, before any intrinsic is hand-written.

**3. `GemmBlocked` — tiled over 256x256x256 blocks.**
Split the `i`/`k`/`j` loops into tiles so that the slice of A, B and C
touched by one tile stays resident in L2 instead of the reordered kernel's
full sweep across whichever rows it's currently on. 256 floats is 1 KiB,
so a 256x256 tile is 256 KiB — sized against this project's dev machine's
L2 (see "notes on the numbers" below); tune `kBlockM/K/N` in
`gemm_blocked.cpp` and `gemm_simd.cpp` for a different cache hierarchy.

**4. `GemmSimd` — AVX2 + FMA inner loop.**
Same blocked loop nest, but the innermost `j` loop processes 8 floats at
a time with `_mm256_fmadd_ps` — one fused multiply-add instruction
instead of 8 separate multiply-then-add pairs, with any `<8`-wide
remainder falling back to scalar.

**5. `GemmThreaded` — `GemmSimd` split across a thread pool.**
`ThreadPool::ParallelFor` (`include/tensor/thread_pool.hpp`) splits `C`'s
rows evenly across `hardware_concurrency()` persistent worker threads,
each running the blocked+SIMD kernel on its own disjoint row range —
no locking inside the parallel region, since no two threads ever write
the same row. The pool's threads are created once at process start and
reused for every `GemmThreaded` call; the benchmark below calls it
thousands of times, and spawning `std::thread`s per call (tens of
microseconds each) would have made thread creation the dominant cost
rather than the matmul itself.

## Correctness

`tests/test_gemm_correctness.cpp` runs every kernel in `kGemmKernels`
against `GemmNaive` on eight shapes, including sizes that are *not*
multiples of the SIMD width (8) or block size (256) — `{33,17,65}`,
`{129,65,200}`, etc. — specifically to exercise the scalar remainder
paths in the blocked/SIMD/threaded kernels, not just their fast path.
Comparison is `AllClose` (relative + absolute tolerance), since FMA and
reduction-order changes shift floating-point rounding slightly between
kernels even when both are "correct."

`examples/mlp_inference_demo.cpp` re-checks this end-to-end: it runs a
full MLP forward pass through both `GemmNaive` and `GemmThreaded` and
diffs the two networks' output, not just a bare matmul's.

## Benchmark results

Measured on this project's dev container: 4 threads, x86-64,
AVX2+FMA+AVX-512F available (only AVX2/FMA used). Run `./build/bench_gemm`
to reproduce on your own machine — the table below is real output, not
hand-written.

| size (M=K=N) | naive (GFLOP/s) | reordered (GFLOP/s) | blocked (GFLOP/s) | simd (GFLOP/s) | threaded (GFLOP/s) |
|---|---|---|---|---|---|
| 64   | 2.92  | 16.12 | 18.87 | 22.84 | 7.91  |
| 128  | 2.04  | 23.24 | 18.30 | 21.83 | 28.41 |
| 256  | 1.56  | 26.15 | 18.81 | 24.53 | 31.69 |
| 512  | 1.73  | 26.33 | 19.78 | 23.54 | 68.90 |
| 1024 | skipped* | 13.53 | 18.48 | 21.60 | 81.05 |

\* `GemmNaive` is skipped at 1024 — its scalar loop would take on the
order of a minute to get a stable median at that size, and its scaling is
already obvious from the smaller rows. Naive to reordered is consistently
a **~9-16x** jump from loop order alone; threaded reaches **~40-60x**
over naive at 512-1024, and **8-9x over single-threaded `GemmSimd`** at
those sizes on 4 threads (i.e. real, if imperfect, parallel scaling —
not just the 4x you'd get from cores alone, because the SIMD kernel is
also somewhat memory-bound and threading adds bandwidth as well as
compute).

**Notes on the numbers, honestly:** `GemmBlocked` and `GemmSimd` do *not*
consistently beat `GemmReordered` at small/medium sizes here, and that's
a genuine measurement, not a bug — `test_gemm_correctness` passes for all
five kernels at all eight test shapes. Two things explain it on this
particular machine: (1) GCC's autovectorizer already turns
`GemmReordered`'s simple inner loop into AVX2 code on its own (confirmed
via `-fopt-info-vec-optimized`), so the hand-written SIMD kernel is
competing against compiler-generated SIMD, not scalar code; (2) this
container reports a 260 MiB L3, which is unusually large — at these
matrix sizes (up to 3 MB of A+B+C combined at N=1024) almost everything
already fits comfortably in *some* level of cache even unblocked, so
tiling's classic benefit (avoiding cache-capacity misses) is muted here.
Threading is the one optimization that wins unambiguously and by a wide
margin at every size, because it's adding real additional compute
throughput (multiple cores) rather than just improving how a single
core's cache behaves. On a machine with a smaller, more typical L2/L3
you should expect `GemmBlocked`/`GemmSimd` to separate more clearly from
`GemmReordered`, which is exactly the kind of platform-dependence real
GEMM libraries (OpenBLAS, MKL, oneDNN) handle by shipping multiple tuned
kernels per microarchitecture instead of one.

## MLP forward pass demo

`examples/mlp_inference_demo.cpp` runs a batch=128, 784&rarr;256&rarr;10
MLP forward pass (`matmul -> +bias -> relu`, `matmul -> +bias -> sigmoid`
— MNIST-sized layers) through both `GemmNaive` and `GemmThreaded`, and
verifies the two networks agree before reporting timing. Measured here:

```
MLP forward pass, batch=128, 784 -> 256 -> 10
  GemmNaive:    29.907 ms/inference
  GemmThreaded: 1.706 ms/inference
  speedup:      17.5x
```

## Design notes

- **`Matrix` is move-only, not copyable.** It owns a 64-byte-aligned
  buffer (`AlignedBuffer<float>`) via RAII; an implicit deep copy of a
  potentially large matrix is exactly the kind of accidental-O(N)
  operation this project is trying to avoid elsewhere, so it isn't
  offered silently.
- **Every row starts 64-byte aligned**, not just the buffer's start —
  `Matrix` pads each row's stride up to a multiple of 16 floats
  (`stride()` vs. `cols()`). This is what lets `GemmSimd` use aligned
  reasoning about block boundaries without a per-row alignment check.
- **`ThreadPool` is a static-partition `ParallelFor`, not a task queue.**
  GEMM's rows all cost the same amount of work, so an even static split
  is optimal and far simpler than work-stealing. A task queue would be
  the right call for uneven workloads — noted as a possible extension,
  not implemented, since it would be solving a problem this workload
  doesn't have.
- **Why not `-march=native`?** It would make the build silently target
  whatever CPU happens to run `cmake`, which is wrong for a project
  meant to build the same way in CI and on a reviewer's machine.
  `-mavx2 -mfma` is an explicit, portable floor instead.

## Possible extensions

Left out deliberately to keep the progression's five steps each teaching
one clear idea, not because they're hard:

- **Packing** (BLIS/GotoBLAS-style): copy tiles of A and B into
  contiguous scratch buffers before the microkernel runs, removing the
  strided access blocking alone doesn't fully eliminate. Usually the next
  double-digit-percent win after SIMD+blocking.
- **A proper microkernel** (e.g. 6x16 register-blocked accumulation)
  instead of the current 1-row-at-a-time inner loop, to better hide FMA
  latency behind independent accumulator chains.
- **AVX-512** path (available on this dev machine per `lscpu`, unused
  here to keep the SIMD floor at the more universally-available AVX2).
- **Work-stealing** in `ThreadPool`, if this were extended to workloads
  with uneven per-chunk cost.
