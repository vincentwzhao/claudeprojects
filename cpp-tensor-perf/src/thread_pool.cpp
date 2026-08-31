#include "tensor/thread_pool.hpp"

#include <algorithm>

namespace tensor {

ThreadPool::ThreadPool(std::size_t num_threads) {
  workers_.reserve(num_threads);
  for (std::size_t i = 0; i < num_threads; ++i)
    workers_.emplace_back(&ThreadPool::WorkerLoop, this, i);
}

ThreadPool::~ThreadPool() {
  {
    std::lock_guard<std::mutex> lock(mu_);
    stop_ = true;
    ++generation_;
  }
  wake_cv_.notify_all();
  for (auto& t : workers_) t.join();
}

void ThreadPool::ParallelFor(
    std::size_t n, const std::function<void(std::size_t, std::size_t)>& body) {
  if (n == 0) return;
  if (workers_.empty()) {
    body(0, n);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(mu_);
    body_ = &body;
    n_ = n;
    pending_ = workers_.size();
    ++generation_;
  }
  wake_cv_.notify_all();

  std::unique_lock<std::mutex> lock(mu_);
  done_cv_.wait(lock, [this] { return pending_ == 0; });
  body_ = nullptr;
}

void ThreadPool::WorkerLoop(std::size_t worker_id) {
  std::size_t seen_generation = 0;
  while (true) {
    std::unique_lock<std::mutex> lock(mu_);
    wake_cv_.wait(
        lock, [&] { return stop_ || generation_ != seen_generation; });
    if (stop_) return;
    seen_generation = generation_;

    const std::size_t n = n_;
    const std::size_t num_workers = workers_.size();
    const auto* body = body_;
    lock.unlock();

    // Static, even split of [0, n) into num_workers contiguous chunks.
    std::size_t chunk = (n + num_workers - 1) / num_workers;
    std::size_t begin = std::min(worker_id * chunk, n);
    std::size_t end = std::min(begin + chunk, n);
    if (begin < end) (*body)(begin, end);

    {
      std::lock_guard<std::mutex> done_lock(mu_);
      if (--pending_ == 0) done_cv_.notify_one();
    }
  }
}

ThreadPool& GlobalThreadPool() {
  static ThreadPool pool(std::max(1u, std::thread::hardware_concurrency()));
  return pool;
}

}  // namespace tensor
