# 10 — Concurrency

## The concept

Multiple threads sharing memory is the source of an entire category of bugs
that don't exist in single-threaded code: races, deadlocks, and visibility
issues. This module builds up the core toolkit in `<thread>`, `<mutex>`,
`<atomic>`, and `<condition_variable>`.

### `std::thread`

```cpp
std::thread t([]{ do_work(); });
t.join();   // block until t finishes — MUST join or detach before t is destroyed
```
A `std::thread` whose destructor runs while still joinable (never joined or
detached) calls `std::terminate` — this is the most common first bug.

### Data races and `std::mutex`

A **data race**: two threads access the same memory, at least one writes,
with no synchronization between them. This is undefined behavior — not
"maybe wrong," genuinely UB, and can manifest as anything from a wrong
value to a crash to nothing observable at all (which is worse, because it
hides until it doesn't).

```cpp
std::mutex m;
int counter = 0;
void increment() {
    std::lock_guard<std::mutex> lock(m);   // RAII: locks on construction,
    ++counter;                              // unlocks on destruction (08-raii)
}
```
`std::lock_guard` is the RAII pattern applied to locking — you cannot
forget to unlock, even if an exception is thrown mid-critical-section.
`std::unique_lock` is the more flexible (movable, unlockable-and-relockable)
version, needed for condition variables.

### `std::atomic`

For simple counters/flags, `std::atomic<T>` gives lock-free (on most
platforms, for suitable `T`) synchronized access without an explicit mutex:

```cpp
std::atomic<int> counter{0};
counter.fetch_add(1, std::memory_order_relaxed);
```

### Condition variables — waiting for a condition, not just a lock

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;

// waiter:
std::unique_lock<std::mutex> lock(m);
cv.wait(lock, [] { return ready; });   // releases lock while waiting, reacquires on wake

// notifier:
{ std::lock_guard<std::mutex> lock(m); ready = true; }
cv.notify_one();
```
Always wait on a *predicate* (the lambda), not just `cv.wait(lock)` alone —
spurious wakeups are allowed by the standard, and the predicate form loops
until the condition is actually true.

### Deadlock

Classic cause: two threads each hold one lock and wait for the other's lock
(lock ordering inversion). Fix by always acquiring multiple locks in a
consistent global order, or using `std::lock`/`std::scoped_lock` which locks
several mutexes atomically without a fixed order requirement.

## Common traps

- Forgetting to `join()`/`detach()` a `std::thread` — `std::terminate`.
- Data races on shared state with no mutex/atomic — UB, and often "works on
  my machine" until it doesn't under load.
- Deadlock from inconsistent lock ordering across threads.
- `cv.wait(lock)` without a predicate — vulnerable to spurious wakeups and
  lost wakeups (notify before wait).
- Capturing a thread's stack-local by reference in a lambda when the thread
  might outlive the caller's scope — dangling reference, the classic
  `01-pointers`/`02-references` bug wearing a concurrency costume.

## Run it

```bash
./10-concurrency-demo
./10-concurrency-exercises
# ./10-concurrency-demo relies on real thread interleaving — output order
# of "unsynchronized" sections will vary run to run; that's the point.
```
