#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace tensor {

// A fixed-size pool of persistent worker threads with a single operation:
// ParallelFor, which statically splits a range into one contiguous chunk
// per worker and blocks until every chunk finishes.
//
// This is intentionally not a general task queue. GemmThreaded calls
// ParallelFor once per GEMM call, potentially thousands of times in a
// benchmark loop; spawning std::threads per call would make thread
// creation (tens of microseconds) dominate the timing for small matrices.
// Keeping the threads alive and only ever paying wait/wake-up costs is
// what makes threading a net win here. See README.md's "why a thread
// pool" section for the numbers that motivated this.
//
// Trade-off: work is split statically and evenly by row count, not by
// remaining work, so an unevenly-loaded task (not the case for GEMM, where
// every row costs the same) would want a work-stealing pool instead. Noted
// as future work in README.md.
class ThreadPool {
 public:
  explicit ThreadPool(std::size_t num_threads);
  ~ThreadPool();

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  std::size_t size() const noexcept { return workers_.size(); }

  // Splits [0, n) into size() contiguous half-open chunks [begin, end) and
  // invokes body(begin, end) for each on a worker thread. Blocks the
  // caller until every chunk has completed.
  void ParallelFor(std::size_t n,
                    const std::function<void(std::size_t, std::size_t)>& body);

 private:
  void WorkerLoop(std::size_t worker_id);

  std::vector<std::thread> workers_;

  std::mutex mu_;
  std::condition_variable wake_cv_;    // workers wait on this for new work
  std::condition_variable done_cv_;    // caller waits on this for completion

  // Shared per-call state, valid only while a ParallelFor call is active.
  const std::function<void(std::size_t, std::size_t)>* body_ = nullptr;
  std::size_t n_ = 0;
  std::size_t generation_ = 0;   // bumped each ParallelFor call
  std::size_t pending_ = 0;      // workers still running this generation
  bool stop_ = false;
};

// Process-wide pool sized to the number of hardware threads, shared by all
// callers of GemmThreaded so repeated GEMM calls reuse the same threads
// instead of paying thread-creation cost each time.
ThreadPool& GlobalThreadPool();

}  // namespace tensor
