#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <vector>

namespace tensor {

// Runs fn() `warmup` times (discarded, lets caches/branch predictors/CPU
// frequency settle) then `iters` times, timing each with
// std::chrono::steady_clock, and returns the median wall-clock seconds.
// Median rather than mean/min: on a shared/CI machine a handful of runs
// can get scheduler-preempted for one bad sample, and the median shrugs
// that off without discarding legitimate variance the way min() does.
template <typename Fn>
double BenchmarkSeconds(Fn&& fn, int warmup = 3, int iters = 10) {
  for (int i = 0; i < warmup; ++i) fn();

  std::vector<double> samples;
  samples.reserve(iters);
  for (int i = 0; i < iters; ++i) {
    auto start = std::chrono::steady_clock::now();
    fn();
    auto end = std::chrono::steady_clock::now();
    samples.push_back(std::chrono::duration<double>(end - start).count());
  }

  std::sort(samples.begin(), samples.end());
  return samples[samples.size() / 2];
}

}  // namespace tensor
