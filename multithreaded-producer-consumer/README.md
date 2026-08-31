# Project 2: Multithreaded Program

A small C++17 producer/consumer pipeline, plus two focused demos that isolate
the failure modes it's built to avoid.

```
Producer threads
      |
   Queue                (bounded, mutex + condition variables)
      |
Consumer threads
```

## Concepts covered

| Concept              | Where |
|-----------------------|-------|
| Mutexes               | `include/blocking_queue.hpp` (`mutex_`), `src/race_condition_demo.cpp` (`MutexProtectedIncrement`) |
| Condition variables    | `include/blocking_queue.hpp` (`not_full_` / `not_empty_`) |
| Race conditions        | `src/race_condition_demo.cpp` (`RaceyIncrement`) |
| Deadlocks              | `src/deadlock_demo.cpp` (`DeadlockProneTransfer` vs. `DeadlockFreeTransfer`) |
| Atomics                | `src/producer_consumer.cpp` (`produced_count`, `consumed_count`, `producers_remaining`), `src/race_condition_demo.cpp` (`AtomicIncrement`) |

## Layout

```
multithreaded-producer-consumer/
  include/blocking_queue.hpp   thread-safe bounded queue
  src/producer_consumer.cpp    main demo: producers -> queue -> consumers
  src/race_condition_demo.cpp  unsynchronized vs. mutex vs. atomic counter
  src/deadlock_demo.cpp        lock-order deadlock vs. std::scoped_lock fix
  CMakeLists.txt
  run_demos.sh                 build + run all three
```

## Build & run

```
./run_demos.sh
```

or manually:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/producer_consumer [num_producers] [num_consumers] [items_per_producer] [queue_capacity]
./build/race_condition_demo
./build/deadlock_demo
```

## 1. Producer/consumer (`producer_consumer.cpp`)

Producer threads generate `WorkItem`s and push them into a bounded
`BlockingQueue`; consumer threads pop and process them. The queue is the only
shared state producers and consumers touch directly.

- **Mutex + condition variables**: `BlockingQueue` holds one `std::mutex` and
  two `std::condition_variable`s. `Push()` blocks on `not_full_` while the
  queue is at capacity; `Pop()` blocks on `not_empty_` while the queue is
  empty. Because both wait predicates are checked under the same mutex,
  producers and consumers never see the queue mid-update - this is what
  prevents the race conditions and lost/duplicated items that a naive
  shared `std::queue` (or busy-polling a plain `bool`/counter) would produce.
- **Shutdown without deadlock**: producers never block waiting on consumers
  directly, and vice versa - they only ever wait on the single queue mutex,
  briefly. When the last producer finishes, it calls `queue.Close()`, which
  wakes every blocked thread; consumers keep draining whatever is left, then
  `Pop()` starts returning `std::nullopt` once the queue is both closed and
  empty, and each consumer thread exits its loop. There's no scenario where a
  producer and a consumer each hold a lock the other needs.
- **Atomics**: `produced_count`, `consumed_count`, and `producers_remaining`
  are `std::atomic`, incremented/decremented from multiple threads with no
  mutex. `producers_remaining.fetch_sub(...) == 1` is the signal exactly one
  producer thread will ever observe, which is what makes "last producer
  closes the queue" safe without a separate lock.
- At the end, the program asserts `produced_count == consumed_count` as a
  correctness check - the queue guarantees every item is delivered exactly
  once.

## 2. Race conditions (`race_condition_demo.cpp`)

Eight threads each increment a shared counter 200,000 times, three ways:

1. **Unsynchronized (`RaceyIncrement`)** - `counter = counter + 1` with no
   lock. Read-modify-write is not one step, so increments from different
   threads clobber each other; the final total is usually well short of the
   expected 1,600,000, and different every run. (The counter is declared
   `volatile` purely to stop the optimizer from proving the loop
   race-free and collapsing it into a single instruction per thread, which
   would otherwise make the bug much harder to reproduce - it is not a fix.)
2. **Mutex-protected (`MutexProtectedIncrement`)** - the increment happens
   inside a `std::lock_guard`, so only one thread can execute it at a time.
   Always correct.
3. **Atomic (`AtomicIncrement`)** - `std::atomic<long>::fetch_add` is a
   single hardware read-modify-write instruction with no lock needed.
   Always correct, and cheaper than a mutex for this simple case.

## 3. Deadlocks (`deadlock_demo.cpp`)

Two threads each need to hold two mutexes, `A` and `B`, at once:

- **`DeadlockProneTransfer`**: thread 1 locks `A` then `B`; thread 2 locks
  `B` then `A`. If both threads acquire their first lock before either
  reaches its second, each blocks forever waiting on a mutex the other
  thread holds. (`std::timed_mutex` + `try_lock_for` is used only so the
  demo can detect and report this instead of hanging the terminal - a real
  `std::mutex` deadlock has no way out short of killing the process.)
- **`DeadlockFreeTransfer`**: both threads acquire the same two mutexes
  with `std::scoped_lock lock(mutex_a, mutex_b)` (or `mutex_b, mutex_a` -
  the order given doesn't matter). `std::scoped_lock`'s multi-mutex
  constructor uses a deadlock-avoidance algorithm internally, so it's safe
  even when different threads name the mutexes in different orders.

The general lesson: either always acquire multiple locks in a fixed global
order, or let the standard library do it for you with `std::scoped_lock`/
`std::lock`.
