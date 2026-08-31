#include "tensor/thread_pool.hpp"

#include <atomic>
#include <numeric>
#include <vector>

#include "check.hpp"

int test_thread_pool() {
  int _tt_failures = 0;
  using tensor::ThreadPool;

  {
    ThreadPool pool(4);
    TT_CHECK(pool.size() == 4);

    std::vector<int> out(1000, -1);
    pool.ParallelFor(out.size(), [&](std::size_t begin, std::size_t end) {
      for (std::size_t i = begin; i < end; ++i) out[i] = static_cast<int>(i);
    });
    for (std::size_t i = 0; i < out.size(); ++i)
      TT_CHECK(out[i] == static_cast<int>(i));
  }

  {
    // n smaller than the thread count: some workers get an empty range
    // and must not touch memory they weren't assigned.
    ThreadPool pool(8);
    std::vector<int> out(3, -1);
    pool.ParallelFor(out.size(), [&](std::size_t begin, std::size_t end) {
      for (std::size_t i = begin; i < end; ++i) out[i] = static_cast<int>(i);
    });
    for (std::size_t i = 0; i < out.size(); ++i)
      TT_CHECK(out[i] == static_cast<int>(i));
  }

  {
    // Repeated calls on the same pool must each see a consistent view
    // (exercises the generation counter that guards against a worker
    // reading stale shared state from a previous call).
    ThreadPool pool(4);
    std::atomic<long> sum{0};
    for (int iter = 0; iter < 50; ++iter) {
      sum = 0;
      pool.ParallelFor(400, [&](std::size_t begin, std::size_t end) {
        long local = 0;
        for (std::size_t i = begin; i < end; ++i) local += static_cast<long>(i);
        sum += local;
      });
      TT_CHECK(sum.load() == (399 * 400) / 2);
    }
  }

  return _tt_failures;
}
