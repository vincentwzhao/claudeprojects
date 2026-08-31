#pragma once

#include <cstddef>

#include "tensor/matrix.hpp"

// Internal-only helper shared between gemm_simd.cpp and gemm_threaded.cpp.
// Not part of the public API in gemm.hpp.
namespace tensor::detail {

// Runs the blocked+SIMD GEMM kernel, but only computes output rows in
// [row_begin, row_end) instead of the full [0, a.rows()). c must already
// be zeroed by the caller — this does not touch rows outside its range,
// which is what lets GemmThreaded call this once per worker thread with
// disjoint, non-overlapping row ranges and no synchronization on c.
void GemmSimdRange(const Matrix& a, const Matrix& b, Matrix& c,
                    std::size_t row_begin, std::size_t row_end);

}  // namespace tensor::detail
