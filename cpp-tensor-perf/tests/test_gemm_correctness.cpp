#include <cstdio>

#include "tensor/gemm.hpp"

#include "check.hpp"

namespace {

// Sizes deliberately include values that are NOT multiples of the 8-lane
// SIMD width or the 64-element block size, to exercise every remainder
// path (scalar SIMD tail, partial blocks) in gemm_blocked/simd/threaded.
struct Shape {
  std::size_t m, k, n;
};
constexpr Shape kShapes[] = {
    {1, 1, 1},    {1, 5, 3},     {5, 1, 4},   {7, 7, 7},
    {16, 16, 16}, {33, 17, 65},  {70, 70, 70}, {129, 65, 200},
};

}  // namespace

int test_gemm_correctness() {
  int _tt_failures = 0;
  using tensor::Matrix;

  for (const auto& shape : kShapes) {
    Matrix a(shape.m, shape.k);
    Matrix b(shape.k, shape.n);
    a.FillRandom(/*seed=*/static_cast<unsigned>(shape.m * 1000 + shape.k));
    b.FillRandom(/*seed=*/static_cast<unsigned>(shape.k * 1000 + shape.n));

    Matrix reference(shape.m, shape.n);
    tensor::GemmNaive(a, b, reference);

    for (const auto& kernel : tensor::kGemmKernels) {
      if (kernel.name == "naive") continue;  // that's the reference itself

      Matrix out(shape.m, shape.n);
      kernel.fn(a, b, out);

      bool ok = out.AllClose(reference);
      if (!ok) {
        std::fprintf(stderr,
                      "  FAIL kernel '%.*s' mismatched naive at shape "
                      "(%zu,%zu,%zu)\n",
                      static_cast<int>(kernel.name.size()), kernel.name.data(),
                      shape.m, shape.k, shape.n);
        ++_tt_failures;
      }
    }
  }

  return _tt_failures;
}
