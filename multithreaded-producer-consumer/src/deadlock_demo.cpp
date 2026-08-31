// Demonstrates a classic lock-ordering deadlock, then the fix.

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

// std::timed_mutex (rather than plain std::mutex) is used only so this
// demo can detect a deadlock and report it instead of hanging the
// terminal forever - a real std::mutex deadlock has no way out short of
// killing the process.
std::timed_mutex mutex_a;
std::timed_mutex mutex_b;

// t1 locks A then B; t2 locks B then A. If both threads acquire their
// first lock before either reaches its second, each blocks forever
// waiting for a mutex the other thread is holding: deadlock.
void DeadlockProneTransfer() {
  bool t1_finished = false;
  bool t2_finished = false;

  std::thread t1([&] {
    std::this_thread::sleep_for(50ms);  // widen the race window
    if (mutex_a.try_lock_for(2s)) {
      std::this_thread::sleep_for(100ms);  // simulate work while holding A
      if (mutex_b.try_lock_for(2s)) {
        t1_finished = true;
        mutex_b.unlock();
      } else {
        std::cout << "[t1] timed out waiting for B - deadlock detected\n";
      }
      mutex_a.unlock();
    }
  });

  std::thread t2([&] {
    std::this_thread::sleep_for(50ms);
    if (mutex_b.try_lock_for(2s)) {
      std::this_thread::sleep_for(100ms);  // simulate work while holding B
      if (mutex_a.try_lock_for(2s)) {
        t2_finished = true;
        mutex_a.unlock();
      } else {
        std::cout << "[t2] timed out waiting for A - deadlock detected\n";
      }
      mutex_b.unlock();
    }
  });

  t1.join();
  t2.join();
  std::cout << "deadlock-prone version: t1_finished=" << t1_finished
             << " t2_finished=" << t2_finished << "\n";
}

// Fixed: std::scoped_lock (2+ mutex overload) acquires every mutex given
// to it using a deadlock-avoidance algorithm internally, so it no longer
// matters that t1 and t2 name the mutexes in opposite order.
void DeadlockFreeTransfer() {
  std::thread t1([&] {
    std::this_thread::sleep_for(50ms);
    std::scoped_lock lock(mutex_a, mutex_b);
    std::this_thread::sleep_for(100ms);
    std::cout << "[t1] holding both locks safely\n";
  });

  std::thread t2([&] {
    std::this_thread::sleep_for(50ms);
    std::scoped_lock lock(mutex_b, mutex_a);  // opposite order - still safe
    std::this_thread::sleep_for(100ms);
    std::cout << "[t2] holding both locks safely\n";
  });

  t1.join();
  t2.join();
  std::cout << "deadlock-free version: both threads completed\n";
}

int main() {
  std::cout << "=== deadlock-prone transfer (timed_mutex used only so this "
               "demo can't actually hang) ===\n";
  DeadlockProneTransfer();

  std::cout << "\n=== deadlock-free transfer (std::scoped_lock) ===\n";
  DeadlockFreeTransfer();
  return 0;
}
