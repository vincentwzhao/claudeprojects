// Producer/consumer demo:
//
//   Producer threads
//         |
//      Queue   (bounded, mutex + condition variables)
//         |
//   Consumer threads
//
// Usage: producer_consumer [num_producers] [num_consumers] [items_per_producer] [queue_capacity]

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <vector>

#include "blocking_queue.hpp"

struct WorkItem {
  int producer_id;
  int sequence;
};

int main(int argc, char** argv) {
  int num_producers = 3;
  int num_consumers = 4;
  int items_per_producer = 20;
  size_t queue_capacity = 8;

  if (argc > 1) num_producers = std::atoi(argv[1]);
  if (argc > 2) num_consumers = std::atoi(argv[2]);
  if (argc > 3) items_per_producer = std::atoi(argv[3]);
  if (argc > 4) queue_capacity = static_cast<size_t>(std::atoi(argv[4]));

  BlockingQueue<WorkItem> queue(queue_capacity);
  std::mutex cout_mutex;

  // Atomics: these counters are incremented from many threads with no lock
  // needed, because fetch_add/fetch_sub is a single hardware read-modify-
  // write instruction - there's no window for another thread to interleave.
  std::atomic<long> produced_count{0};
  std::atomic<long> consumed_count{0};
  std::atomic<int> producers_remaining{num_producers};

  auto producer_fn = [&](int id) {
    std::mt19937 rng(std::random_device{}() + static_cast<unsigned>(id));
    std::uniform_int_distribution<int> delay_ms(1, 5);
    for (int i = 0; i < items_per_producer; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms(rng)));
      queue.Push(WorkItem{id, i});
      produced_count.fetch_add(1, std::memory_order_relaxed);
    }
    // The last producer to finish closes the queue so idle consumers can
    // stop waiting. fetch_sub returning 1 means this call just brought the
    // counter to 0 - exactly one thread will ever see that value.
    if (producers_remaining.fetch_sub(1, std::memory_order_acq_rel) == 1) {
      queue.Close();
    }
  };

  auto consumer_fn = [&](int id) {
    while (auto item = queue.Pop()) {
      consumed_count.fetch_add(1, std::memory_order_relaxed);
      std::ostringstream out;
      out << "[consumer " << id << "] processed item " << item->sequence
          << " from producer " << item->producer_id << "\n";
      std::lock_guard<std::mutex> lock(cout_mutex);
      std::cout << out.str();
    }
  };

  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;
  for (int i = 0; i < num_producers; ++i) producers.emplace_back(producer_fn, i);
  for (int i = 0; i < num_consumers; ++i) consumers.emplace_back(consumer_fn, i);

  for (auto& t : producers) t.join();
  for (auto& t : consumers) t.join();

  std::cout << "\nproduced=" << produced_count << " consumed=" << consumed_count << "\n";
  if (produced_count != consumed_count) {
    std::cerr << "MISMATCH: items lost or duplicated\n";
    return 1;
  }
  std::cout << "OK: every produced item was consumed exactly once\n";
  return 0;
}
