// 10-concurrency/exercises.cpp
#include "test.hpp"
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <numeric>

// TODO 1: spawn `num_threads` threads, each adding `per_thread` to *total
// (a shared int), protected by `m` so the final result is always correct.
// Join every thread before returning.
void parallel_sum_mutex(int* total, std::mutex& m, int num_threads, int per_thread) {
    // your code here
}

// TODO 2: same idea, but using std::atomic<int> instead of a mutex.
void parallel_sum_atomic(std::atomic<int>& total, int num_threads, int per_thread) {
    // your code here
}

// TODO 3: each thread i (0..num_threads-1) should compute i*i and store it
// into results[i] — no shared mutable state contention here since each
// thread writes a distinct index, so no lock is needed. Spawn the threads,
// have them fill results, then join all of them.
void parallel_squares(std::vector<int>& results, int num_threads) {
    // your code here (results is already sized to num_threads before this is called)
}

int main() {
    int total = 0;
    std::mutex m;
    parallel_sum_mutex(&total, m, 8, 1000);
    CHECK_EQ(total, 8000);

    std::atomic<int> atomic_total{0};
    parallel_sum_atomic(atomic_total, 8, 1000);
    CHECK_EQ(atomic_total.load(), 8000);

    std::vector<int> results(6, 0);
    parallel_squares(results, 6);
    for (int i = 0; i < 6; ++i) {
        CHECK_EQ(results[i], i * i);
    }

    TEST_SUMMARY();
}
