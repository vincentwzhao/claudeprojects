#include "tensor/gemm.hpp"
#include "tensor/gemm_detail.hpp"
#include "tensor/thread_pool.hpp"

namespace tensor {

// GemmSimd's row range split evenly across GlobalThreadPool()'s workers.
// Each worker owns a disjoint slice of C's rows, so there's no
// synchronization inside the parallel region at all — the only sync cost
// is ParallelFor's single wake/join per call.
//
// c.Fill(0.0f) runs single-threaded before the split. That's O(M*N) work
// against O(M*N*K) for the matmul itself, so for any K worth parallelizing
// over it's a rounding error; see README.md's Amdahl's-law note for where
// that stops being true.
void GemmThreaded(const Matrix& a, const Matrix& b, Matrix& c) {
  c.Fill(0.0f);
  GlobalThreadPool().ParallelFor(
      a.rows(), [&](std::size_t row_begin, std::size_t row_end) {
        detail::GemmSimdRange(a, b, c, row_begin, row_end);
      });
}

}  // namespace tensor
