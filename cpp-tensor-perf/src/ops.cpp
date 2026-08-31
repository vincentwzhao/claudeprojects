#include "tensor/ops.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <immintrin.h>

namespace tensor::ops {

namespace {
constexpr std::size_t kLanes = 8;
}

void Add(const Matrix& a, const Matrix& b, Matrix& out) {
  assert(a.rows() == b.rows() && a.cols() == b.cols());
  assert(a.rows() == out.rows() && a.cols() == out.cols());

  for (std::size_t r = 0; r < a.rows(); ++r) {
    const float* a_row = a.row(r);
    const float* b_row = b.row(r);
    float* out_row = out.row(r);

    std::size_t c = 0;
    for (; c + kLanes <= a.cols(); c += kLanes) {
      __m256 va = _mm256_loadu_ps(a_row + c);
      __m256 vb = _mm256_loadu_ps(b_row + c);
      _mm256_storeu_ps(out_row + c, _mm256_add_ps(va, vb));
    }
    for (; c < a.cols(); ++c) out_row[c] = a_row[c] + b_row[c];
  }
}

void AddBiasInPlace(Matrix& m, const Matrix& bias_row) {
  assert(bias_row.rows() == 1 && bias_row.cols() == m.cols());

  const float* bias = bias_row.row(0);
  for (std::size_t r = 0; r < m.rows(); ++r) {
    float* row = m.row(r);
    std::size_t c = 0;
    for (; c + kLanes <= m.cols(); c += kLanes) {
      __m256 vr = _mm256_loadu_ps(row + c);
      __m256 vb = _mm256_loadu_ps(bias + c);
      _mm256_storeu_ps(row + c, _mm256_add_ps(vr, vb));
    }
    for (; c < m.cols(); ++c) row[c] += bias[c];
  }
}

void ReluInPlace(Matrix& m) {
  const __m256 zero = _mm256_setzero_ps();
  for (std::size_t r = 0; r < m.rows(); ++r) {
    float* row = m.row(r);
    std::size_t c = 0;
    for (; c + kLanes <= m.cols(); c += kLanes) {
      __m256 v = _mm256_loadu_ps(row + c);
      _mm256_storeu_ps(row + c, _mm256_max_ps(v, zero));
    }
    for (; c < m.cols(); ++c) row[c] = std::max(row[c], 0.0f);
  }
}

void SigmoidInPlace(Matrix& m) {
  for (std::size_t r = 0; r < m.rows(); ++r) {
    float* row = m.row(r);
    for (std::size_t c = 0; c < m.cols(); ++c)
      row[c] = 1.0f / (1.0f + std::exp(-row[c]));
  }
}

}  // namespace tensor::ops
