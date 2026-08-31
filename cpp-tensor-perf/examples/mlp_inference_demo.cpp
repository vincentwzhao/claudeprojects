// Ties gemm.hpp and ops.hpp together into what this library actually
// exists to speed up: a forward pass through a small fully-connected
// network. Shapes (batch=128, 784 -> 256 -> 10) match an MNIST-sized MLP.
//
// Runs the same forward pass twice, once wired to GemmNaive and once to
// GemmThreaded, and reports the speedup end-to-end rather than per-kernel
// — this is the number that would actually matter if this were serving
// inference requests.
#include <cstdio>

#include "tensor/gemm.hpp"
#include "tensor/matrix.hpp"
#include "tensor/ops.hpp"
#include "tensor/timer.hpp"

namespace {

constexpr std::size_t kBatch = 128;
constexpr std::size_t kInput = 784;
constexpr std::size_t kHidden = 256;
constexpr std::size_t kOutput = 10;

struct Mlp {
  tensor::Matrix w1{kInput, kHidden};
  tensor::Matrix b1{1, kHidden};
  tensor::Matrix w2{kHidden, kOutput};
  tensor::Matrix b2{1, kOutput};

  Mlp() {
    w1.FillRandom(10);
    b1.FillRandom(11);
    w2.FillRandom(12);
    b2.FillRandom(13);
  }
};

// x: (batch x kInput) -> (batch x kOutput), using `gemm` for both layers'
// matmuls.
tensor::Matrix Forward(const Mlp& net, const tensor::Matrix& x,
                       tensor::GemmFn gemm) {
  tensor::Matrix hidden(kBatch, kHidden);
  gemm(x, net.w1, hidden);
  tensor::ops::AddBiasInPlace(hidden, net.b1);
  tensor::ops::ReluInPlace(hidden);

  tensor::Matrix out(kBatch, kOutput);
  gemm(hidden, net.w2, out);
  tensor::ops::AddBiasInPlace(out, net.b2);
  tensor::ops::SigmoidInPlace(out);
  return out;
}

}  // namespace

int main() {
  Mlp net;
  tensor::Matrix x(kBatch, kInput);
  x.FillRandom(7);

  // Sanity check: both gemm implementations must agree on the actual
  // output, not just be fast.
  tensor::Matrix naive_out = Forward(net, x, &tensor::GemmNaive);
  tensor::Matrix threaded_out = Forward(net, x, &tensor::GemmThreaded);
  if (!naive_out.AllClose(threaded_out)) {
    std::fprintf(stderr,
                 "mismatch between GemmNaive and GemmThreaded forward "
                 "pass output\n");
    return 1;
  }

  double naive_seconds = tensor::BenchmarkSeconds(
      [&] { Forward(net, x, &tensor::GemmNaive); }, /*warmup=*/3, /*iters=*/20);
  double threaded_seconds = tensor::BenchmarkSeconds(
      [&] { Forward(net, x, &tensor::GemmThreaded); }, /*warmup=*/3,
      /*iters=*/20);

  std::printf("MLP forward pass, batch=%zu, %zu -> %zu -> %zu\n", kBatch,
              kInput, kHidden, kOutput);
  std::printf("  GemmNaive:    %.3f ms/inference\n", naive_seconds * 1000.0);
  std::printf("  GemmThreaded: %.3f ms/inference\n",
              threaded_seconds * 1000.0);
  std::printf("  speedup:      %.1fx\n", naive_seconds / threaded_seconds);
  return 0;
}
