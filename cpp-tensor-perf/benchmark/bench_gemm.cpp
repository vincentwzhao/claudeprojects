// Benchmarks every kernel in tensor::kGemmKernels across a range of
// square matrix sizes and prints a markdown table (GFLOP/s per kernel per
// size) straight to stdout, so a run's output can be pasted directly into
// README.md's benchmark section.
//
// GemmNaive is skipped above kMaxNaiveSize: its O(n^3) scalar loop at
// n=1024 takes on the order of a minute for enough iterations to get a
// stable median, and its scaling relative to the other kernels is already
// obvious from the smaller sizes.
#include <cstdio>
#include <vector>

#include "tensor/gemm.hpp"
#include "tensor/thread_pool.hpp"
#include "tensor/timer.hpp"

namespace {

constexpr std::size_t kMaxNaiveSize = 512;

struct SizeConfig {
  std::size_t n;
  int warmup;
  int iters;
};

const std::vector<SizeConfig> kSizes = {
    {64, 5, 20},  {128, 3, 15}, {256, 3, 10}, {512, 2, 6}, {1024, 1, 3},
};

double Gflops(std::size_t m, std::size_t k, std::size_t n, double seconds) {
  double flops = 2.0 * static_cast<double>(m) * static_cast<double>(k) *
                 static_cast<double>(n);
  return flops / seconds / 1e9;
}

}  // namespace

int main() {
  std::printf("Threads available: %zu\n\n", tensor::GlobalThreadPool().size());

  std::printf("| size (M=K=N) |");
  for (const auto& k : tensor::kGemmKernels)
    std::printf(" %.*s (GFLOP/s) |", static_cast<int>(k.name.size()), k.name.data());
  std::printf("\n|---|");
  for (std::size_t i = 0; i < tensor::kGemmKernels.size(); ++i) std::printf("---|");
  std::printf("\n");

  for (const auto& cfg : kSizes) {
    tensor::Matrix a(cfg.n, cfg.n), b(cfg.n, cfg.n), c(cfg.n, cfg.n);
    a.FillRandom(1);
    b.FillRandom(2);

    std::printf("| %zu | ", cfg.n);
    for (const auto& kernel : tensor::kGemmKernels) {
      if (kernel.name == "naive" && cfg.n > kMaxNaiveSize) {
        std::printf("skipped | ");
        continue;
      }
      double seconds = tensor::BenchmarkSeconds(
          [&] { kernel.fn(a, b, c); }, cfg.warmup, cfg.iters);
      std::printf("%.2f | ", Gflops(cfg.n, cfg.n, cfg.n, seconds));
      std::fflush(stdout);
    }
    std::printf("\n");
  }

  return 0;
}
