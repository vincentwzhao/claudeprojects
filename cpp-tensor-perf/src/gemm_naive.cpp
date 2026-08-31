#include "tensor/gemm.hpp"

namespace tensor {

// i-j-k order: for each output element C(i,j), walk k straight through.
// A(i,k) is unit-stride but B(k,j) jumps by a full row (stride() floats)
// on every k, so the inner loop touches a new cache line almost every
// iteration once K exceeds a few cache lines. This is the baseline every
// other kernel is measured and checked against.
void GemmNaive(const Matrix& a, const Matrix& b, Matrix& c) {
  const std::size_t M = a.rows();
  const std::size_t K = a.cols();
  const std::size_t N = b.cols();

  for (std::size_t i = 0; i < M; ++i) {
    for (std::size_t j = 0; j < N; ++j) {
      float sum = 0.0f;
      for (std::size_t k = 0; k < K; ++k) sum += a(i, k) * b(k, j);
      c(i, j) = sum;
    }
  }
}

}  // namespace tensor
