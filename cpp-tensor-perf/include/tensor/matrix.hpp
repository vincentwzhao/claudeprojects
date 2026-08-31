#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <ostream>
#include <random>

#include "tensor/aligned_buffer.hpp"

namespace tensor {

// Dense, row-major, 64-byte-aligned matrix of float. Every row also starts
// at a 64-byte boundary (via padded_cols_), which keeps SIMD loads on any
// row aligned without needing a per-row alignment check in the kernels.
class Matrix {
 public:
  Matrix() = default;

  Matrix(std::size_t rows, std::size_t cols)
      : rows_(rows),
        cols_(cols),
        padded_cols_(PadCols(cols)),
        buf_(rows * PadCols(cols)) {
    std::fill_n(buf_.get(), rows_ * padded_cols_, 0.0f);
  }

  std::size_t rows() const noexcept { return rows_; }
  std::size_t cols() const noexcept { return cols_; }
  // Stride between rows, in elements. >= cols(); use this, not cols(),
  // when walking memory directly so SIMD kernels stay row-aligned.
  std::size_t stride() const noexcept { return padded_cols_; }

  float& operator()(std::size_t r, std::size_t c) noexcept {
    return buf_.get()[r * padded_cols_ + c];
  }
  float operator()(std::size_t r, std::size_t c) const noexcept {
    return buf_.get()[r * padded_cols_ + c];
  }

  float* data() noexcept { return buf_.get(); }
  const float* data() const noexcept { return buf_.get(); }

  // Row pointer, guaranteed 64-byte aligned.
  float* row(std::size_t r) noexcept { return buf_.get() + r * padded_cols_; }
  const float* row(std::size_t r) const noexcept {
    return buf_.get() + r * padded_cols_;
  }

  void Fill(float value) {
    for (std::size_t r = 0; r < rows_; ++r)
      std::fill_n(row(r), cols_, value);
  }

  void FillRandom(unsigned seed, float lo = -1.0f, float hi = 1.0f) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(lo, hi);
    for (std::size_t r = 0; r < rows_; ++r)
      for (std::size_t c = 0; c < cols_; ++c) (*this)(r, c) = dist(rng);
  }

  // Elementwise approximate equality, used by correctness tests to compare
  // an optimized kernel's output against the naive reference. Relative
  // tolerance accounts for FMA/reduction-order changes shifting
  // floating-point rounding between kernels.
  bool AllClose(const Matrix& other, float rel_tol = 1e-3f,
                float abs_tol = 1e-4f) const {
    if (rows_ != other.rows_ || cols_ != other.cols_) return false;
    for (std::size_t r = 0; r < rows_; ++r) {
      for (std::size_t c = 0; c < cols_; ++c) {
        float a = (*this)(r, c);
        float b = other(r, c);
        float diff = std::fabs(a - b);
        if (diff > abs_tol + rel_tol * std::fabs(b)) return false;
      }
    }
    return true;
  }

  friend std::ostream& operator<<(std::ostream& os, const Matrix& m) {
    os << std::fixed << std::setprecision(3);
    for (std::size_t r = 0; r < m.rows_; ++r) {
      for (std::size_t c = 0; c < m.cols_; ++c) os << m(r, c) << ' ';
      os << '\n';
    }
    return os;
  }

 private:
  static std::size_t PadCols(std::size_t cols) {
    constexpr std::size_t kLane = 64 / sizeof(float);  // 16 floats
    return (cols + kLane - 1) / kLane * kLane;
  }

  std::size_t rows_ = 0;
  std::size_t cols_ = 0;
  std::size_t padded_cols_ = 0;
  AlignedBuffer<float> buf_;
};

}  // namespace tensor
