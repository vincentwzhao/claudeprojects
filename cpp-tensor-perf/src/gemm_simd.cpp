#include <algorithm>
#include <immintrin.h>

#include "tensor/gemm.hpp"
#include "tensor/gemm_detail.hpp"

namespace tensor {

namespace {
constexpr std::size_t kBlockM = 256;
constexpr std::size_t kBlockK = 256;
constexpr std::size_t kBlockN = 256;
constexpr std::size_t kLanes = 8;  // floats per AVX2 256-bit register
}  // namespace

namespace detail {

// GemmBlocked's innermost j loop, vectorized: instead of one
// multiply-add per j, _mm256_fmadd_ps does 8 in a single instruction
// (c[j..j+8) += a_ik * b[j..j+8), fused so there's no separate rounding
// step between the multiply and the add). Loads/stores are unaligned
// (loadu/storeu) since a block's j range isn't guaranteed to start on a
// 32-byte boundary once N isn't a multiple of kBlockN; on Haswell-and-
// later this costs effectively nothing when the address happens to be
// aligned anyway. Any remainder < 8 columns at the end of a block falls
// back to scalar.
//
// Only rows in [row_begin, row_end) are touched, so GemmThreaded can run
// this on disjoint row ranges from multiple threads with no locking.
void GemmSimdRange(const Matrix& a, const Matrix& b, Matrix& c,
                    std::size_t row_begin, std::size_t row_end) {
  const std::size_t K = a.cols();
  const std::size_t N = b.cols();

  for (std::size_t ii = (row_begin / kBlockM) * kBlockM; ii < row_end;
       ii += kBlockM) {
    const std::size_t i_start = std::max(ii, row_begin);
    const std::size_t i_end = std::min(ii + kBlockM, row_end);
    if (i_start >= i_end) continue;

    for (std::size_t kk = 0; kk < K; kk += kBlockK) {
      const std::size_t k_end = std::min(kk + kBlockK, K);
      for (std::size_t jj = 0; jj < N; jj += kBlockN) {
        const std::size_t j_end = std::min(jj + kBlockN, N);
        const std::size_t j_simd_end =
            jj + ((j_end - jj) / kLanes) * kLanes;

        for (std::size_t i = i_start; i < i_end; ++i) {
          float* c_row = c.row(i);
          for (std::size_t k = kk; k < k_end; ++k) {
            const __m256 a_ik = _mm256_set1_ps(a(i, k));
            const float* b_row = b.row(k);

            std::size_t j = jj;
            for (; j < j_simd_end; j += kLanes) {
              __m256 c_vec = _mm256_loadu_ps(c_row + j);
              __m256 b_vec = _mm256_loadu_ps(b_row + j);
              c_vec = _mm256_fmadd_ps(a_ik, b_vec, c_vec);
              _mm256_storeu_ps(c_row + j, c_vec);
            }
            const float a_ik_scalar = a(i, k);
            for (; j < j_end; ++j) c_row[j] += a_ik_scalar * b_row[j];
          }
        }
      }
    }
  }
}

}  // namespace detail

void GemmSimd(const Matrix& a, const Matrix& b, Matrix& c) {
  c.Fill(0.0f);
  detail::GemmSimdRange(a, b, c, 0, a.rows());
}

}  // namespace tensor
