#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

// Thread-safe bounded queue used to hand work off between producer and
// consumer threads.
//
// Two condition variables coordinate the two ways a thread can block:
// producers wait on `not_full_` when the queue is at capacity, consumers
// wait on `not_empty_` when the queue is empty. Both share `mutex_`, so a
// producer's Push() and a consumer's Pop() never observe `queue_` mid-update
// - that's what rules out the classic race condition where two threads
// read-modify-write shared state at the same time.
template <typename T>
class BlockingQueue {
 public:
  explicit BlockingQueue(size_t capacity) : capacity_(capacity) {}

  // Blocks until there is room in the queue or the queue is closed.
  // Returns false (and drops the item) if the queue was closed - producers
  // should stop calling Push() once that happens.
  bool Push(T item) {
    std::unique_lock<std::mutex> lock(mutex_);
    not_full_.wait(lock, [this] { return queue_.size() < capacity_ || closed_; });
    if (closed_) return false;
    queue_.push(std::move(item));
    lock.unlock();
    not_empty_.notify_one();
    return true;
  }

  // Blocks until an item is available. Once the queue has been closed and
  // fully drained, returns std::nullopt so consumers know to exit.
  std::optional<T> Pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    not_empty_.wait(lock, [this] { return !queue_.empty() || closed_; });
    if (queue_.empty()) return std::nullopt;
    T item = std::move(queue_.front());
    queue_.pop();
    lock.unlock();
    not_full_.notify_one();
    return item;
  }

  // Wakes every blocked producer/consumer and marks the queue closed.
  // Call once all producers have finished pushing; consumers keep draining
  // whatever is left before Pop() starts returning nullopt.
  void Close() {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      closed_ = true;
    }
    not_full_.notify_all();
    not_empty_.notify_all();
  }

 private:
  std::mutex mutex_;
  std::condition_variable not_full_;
  std::condition_variable not_empty_;
  std::queue<T> queue_;
  const size_t capacity_;
  bool closed_ = false;
};
