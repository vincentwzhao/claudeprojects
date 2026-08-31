#include <algorithm>

#include "tensor/gemm.hpp"

namespace tensor {

// 256x256x256 was picked empirically against this machine's cache sizes
// (see README.md's "notes on the numbers"): a 256x256 float tile is
// 256 KiB, so the working set touched by one (ii, kk, jj) block — a few
// such tiles of A, B and C, plus slack — sits inside a several-MiB L2
// instead of streaming the whole matrix through cache on every pass.
// Retune these three constants if you're benchmarking on a machine with a
// smaller L2 (try 64 first).
constexpr std::size_t kBlockM = 256;
constexpr std::size_t kBlockK = 256;
constexpr std::size_t kBlockN = 256;

// GemmReordered's i-k-j loop nest, additionally tiled over BM x BK x BN
// blocks. For each (ii, kk, jj) block, GemmReordered's inner loops run
// only inside that block's range, so the slice of A/B/C actually touched
// while it's hot stays cache-resident instead of streaming through all of
// A and B's rows for every output tile.
void GemmBlocked(const Matrix& a, const Matrix& b, Matrix& c) {
  const std::size_t M = a.rows();
  const std::size_t K = a.cols();
  const std::size_t N = b.cols();

  c.Fill(0.0f);
  for (std::size_t ii = 0; ii < M; ii += kBlockM) {
    const std::size_t i_end = std::min(ii + kBlockM, M);
    for (std::size_t kk = 0; kk < K; kk += kBlockK) {
      const std::size_t k_end = std::min(kk + kBlockK, K);
      for (std::size_t jj = 0; jj < N; jj += kBlockN) {
        const std::size_t j_end = std::min(jj + kBlockN, N);

        for (std::size_t i = ii; i < i_end; ++i) {
          for (std::size_t k = kk; k < k_end; ++k) {
            const float a_ik = a(i, k);
            for (std::size_t j = jj; j < j_end; ++j)
              c(i, j) += a_ik * b(k, j);
          }
        }
      }
    }
  }
}

}  // namespace tensor
