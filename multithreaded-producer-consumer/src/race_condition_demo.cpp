// Demonstrates a race condition on a shared counter, then two fixes:
// a mutex-protected critical section, and a lock-free atomic.

#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

constexpr int kThreads = 8;
constexpr int kIncrementsPerThread = 200000;

// No synchronization at all: `counter++` is really read, add, write -
// three separate steps. When two threads interleave those steps on the
// same counter, one thread's increment can be overwritten by the other's,
// so the final total is usually less than kThreads * kIncrementsPerThread,
// and by a different amount every run.
//
// `counter` is declared volatile here only to stop the optimizer from
// proving the loop race-free and collapsing it into one add instruction
// per thread (legal, because unsynchronized access is undefined behavior,
// so the compiler is allowed to assume no other thread touches it). volatile
// forces a real memory read and write every iteration, which is what
// actually recreates the interleaving this demo is trying to show - it is
// not itself a fix for the race.
long RaceyIncrement() {
  volatile long counter = 0;
  std::vector<std::thread> threads;
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([&counter] {
      for (int j = 0; j < kIncrementsPerThread; ++j) {
        counter = counter + 1;  // NOT thread-safe: read, add, write.
      }
    });
  }
  for (auto& t : threads) t.join();
  return counter;
}

// Fixed with a mutex: only one thread can be inside the critical section
// at a time, so the read-modify-write can never be split by another thread.
long MutexProtectedIncrement() {
  long counter = 0;
  std::mutex mutex;
  std::vector<std::thread> threads;
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([&counter, &mutex] {
      for (int j = 0; j < kIncrementsPerThread; ++j) {
        std::lock_guard<std::mutex> lock(mutex);
        counter++;
      }
    });
  }
  for (auto& t : threads) t.join();
  return counter;
}

// Fixed with an atomic: fetch_add compiles to a single hardware
// read-modify-write instruction, so there's no window where two threads
// can both read the same old value.
long AtomicIncrement() {
  std::atomic<long> counter{0};
  std::vector<std::thread> threads;
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([&counter] {
      for (int j = 0; j < kIncrementsPerThread; ++j) {
        counter.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }
  for (auto& t : threads) t.join();
  return counter.load();
}

int main() {
  const long expected = static_cast<long>(kThreads) * kIncrementsPerThread;
  std::cout << "expected total: " << expected << "\n\n";

  long racey = RaceyIncrement();
  std::cout << "racey counter++            -> " << racey;
  std::cout << (racey == expected ? "  (got lucky this run, try again)" : "  (lost updates!)")
             << "\n";

  long mutexed = MutexProtectedIncrement();
  std::cout << "mutex-protected counter++  -> " << mutexed << "\n";

  long atomic = AtomicIncrement();
  std::cout << "atomic fetch_add           -> " << atomic << "\n";

  return 0;
}
