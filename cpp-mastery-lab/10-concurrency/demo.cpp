// 10-concurrency/demo.cpp
#include <cstdio>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <vector>

constexpr int kThreads = 8;
constexpr int kIncrementsPerThread = 200000;

void racy_increment(int& counter) {
    for (int i = 0; i < kIncrementsPerThread; ++i) {
        ++counter;   // read-modify-write, NOT atomic — a data race under threads
    }
}

void race_condition_demo() {
    int counter = 0;
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(racy_increment, std::ref(counter));
    }
    for (auto& t : threads) t.join();   // must join before threads are destroyed

    int expected = kThreads * kIncrementsPerThread;
    printf("  expected %d, got %d %s\n", expected, counter,
           counter == expected ? "(got lucky this run)" : "(lost updates from the race)");
}

void mutex_protected_increment(int& counter, std::mutex& m) {
    for (int i = 0; i < kIncrementsPerThread; ++i) {
        std::lock_guard<std::mutex> lock(m);   // RAII: locked here, unlocked at loop end
        ++counter;
    }
}

void mutex_demo() {
    int counter = 0;
    std::mutex m;
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back(mutex_protected_increment, std::ref(counter), std::ref(m));
    }
    for (auto& t : threads) t.join();

    int expected = kThreads * kIncrementsPerThread;
    printf("  expected %d, got %d (always matches — mutex serializes access)\n", expected, counter);
}

void atomic_demo() {
    std::atomic<int> counter{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&counter] {
            for (int j = 0; j < kIncrementsPerThread; ++j) {
                counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : threads) t.join();

    int expected = kThreads * kIncrementsPerThread;
    printf("  expected %d, got %d (always matches — lock-free, cheaper than a mutex here)\n",
           expected, counter.load());
}

void condition_variable_demo() {
    std::mutex m;
    std::condition_variable cv;
    bool ready = false;
    int produced_value = 0;

    std::thread producer([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        {
            std::lock_guard<std::mutex> lock(m);
            produced_value = 99;
            ready = true;
        }
        cv.notify_one();
        printf("  producer: notified consumer\n");
    });

    std::thread consumer([&] {
        std::unique_lock<std::mutex> lock(m);
        cv.wait(lock, [&ready] { return ready; });   // wait on a predicate, not just a signal
        printf("  consumer: woke up, saw produced_value=%d\n", produced_value);
    });

    producer.join();
    consumer.join();
}

int main() {
    printf("-- data race: unsynchronized increments across %d threads --\n", kThreads);
    race_condition_demo();

    printf("\n-- fixed with std::mutex + lock_guard (RAII) --\n");
    mutex_demo();

    printf("\n-- fixed with std::atomic (lock-free) --\n");
    atomic_demo();

    printf("\n-- condition variable: producer/consumer handoff --\n");
    condition_variable_demo();

    return 0;
}
