# 10 — Concurrency: solutions

```cpp
void parallel_sum_mutex(int* total, std::mutex& m, int num_threads, int per_thread) {
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([total, &m, per_thread] {
            std::lock_guard<std::mutex> lock(m);
            *total += per_thread;
        });
    }
    for (auto& t : threads) t.join();
}

void parallel_sum_atomic(std::atomic<int>& total, int num_threads, int per_thread) {
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&total, per_thread] {
            total.fetch_add(per_thread, std::memory_order_relaxed);
        });
    }
    for (auto& t : threads) t.join();
}

void parallel_squares(std::vector<int>& results, int num_threads) {
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&results, i] {
            results[i] = i * i;   // distinct index per thread -> no race, no lock needed
        });
    }
    for (auto& t : threads) t.join();
}
```

Notes:
- `parallel_sum_mutex` locks around the *entire* `+= per_thread`, not just
  part of it — a lock must cover every read-modify-write step of a critical
  section, not just the final write.
- `parallel_squares` needs no synchronization at all because each thread
  touches a distinct `results[i]` — different memory, so there's no race by
  definition. Recognizing "this doesn't need a lock" is as important as
  knowing how to add one; unnecessary locking just adds contention.
- Capturing `i` by value (`[&results, i]`) in the lambda, not by reference,
  matters: `i` is the loop variable and would otherwise be a dangling/racy
  reference once the loop moves on or ends.
