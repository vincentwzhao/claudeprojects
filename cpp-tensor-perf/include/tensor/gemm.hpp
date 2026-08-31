#pragma once

#include <array>
#include <string_view>

#include "tensor/matrix.hpp"

// GEMM: C = A * B, where A is (M x K), B is (K x N), C is (M x N).
// C must already be sized to (A.rows(), B.cols()); each kernel overwrites
// it (no accumulation into a pre-existing C).
//
// The five kernels below are the same algorithm at increasing levels of
// optimization; see README.md for the reasoning behind each step and
// measured speedups. Every kernel is verified against gemm_naive in
// tests/test_gemm_correctness.cpp before it's allowed to appear in a
// benchmark number.
namespace tensor {

// Textbook i-j-k triple loop. O(N) stride-K access on B's inner loop
// means the inner loop touches a new cache line on almost every step.
// This is the reference implementation everything else is checked against.
void GemmNaive(const Matrix& a, const Matrix& b, Matrix& c);

// Same loop nest, reordered to i-k-j. B and C are now walked row-wise
// (unit stride) in the innermost loop; A is accessed one scalar at a time
// per k. No new memory traffic, just a better access pattern — free
// speedup from reordering alone.
void GemmReordered(const Matrix& a, const Matrix& b, Matrix& c);

// i-k-j loop nest, tiled over 64x64x64 blocks so the working set for one
// tile of A, B and C fits in L2 cache instead of thrashing on matrices
// too big for cache.
void GemmBlocked(const Matrix& a, const Matrix& b, Matrix& c);

// Blocked kernel with the inner j loop vectorized: 8 floats/cycle via
// AVX2 256-bit registers, using FMA (_mm256_fmadd_ps) to fuse the
// multiply-add into one instruction.
void GemmSimd(const Matrix& a, const Matrix& b, Matrix& c);

// GemmSimd's row range split across a persistent thread pool
// (tensor::GlobalThreadPool()), so the blocked+SIMD kernel runs on all
// cores instead of one.
void GemmThreaded(const Matrix& a, const Matrix& b, Matrix& c);

using GemmFn = void (*)(const Matrix&, const Matrix&, Matrix&);

struct GemmKernel {
  std::string_view name;
  GemmFn fn;
};

// All kernels, naive first, most optimized last — used by the benchmark
// and correctness test drivers so both iterate the same list.
inline constexpr std::array<GemmKernel, 5> kGemmKernels = {{
    {"naive", &GemmNaive},
    {"reordered", &GemmReordered},
    {"blocked", &GemmBlocked},
    {"simd", &GemmSimd},
    {"threaded", &GemmThreaded},
}};

}  // namespace tensor
