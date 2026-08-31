#include "tensor/gemm.hpp"

namespace tensor {

// Same three loops as GemmNaive, reordered i-k-j. Hoisting k out one level
// means the innermost loop now walks B(k, *) and C(i, *) both unit-stride,
// row by row — no algorithmic change, no extra memory traffic, just a
// cache-friendly order. This alone is typically a multi-x speedup over
// GemmNaive on any matrix bigger than L1.
void GemmReordered(const Matrix& a, const Matrix& b, Matrix& c) {
  const std::size_t M = a.rows();
  const std::size_t K = a.cols();
  const std::size_t N = b.cols();

  c.Fill(0.0f);
  for (std::size_t i = 0; i < M; ++i) {
    for (std::size_t k = 0; k < K; ++k) {
      const float a_ik = a(i, k);
      for (std::size_t j = 0; j < N; ++j) c(i, j) += a_ik * b(k, j);
    }
  }
}

}  // namespace tensor
