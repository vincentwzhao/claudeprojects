// bench_main.cpp — real throughput numbers for malloc/new vs the three
// allocators in this repo, across four workloads chosen to show where
// each one actually wins (and where it doesn't — this prints whatever it
// measures, not what would make the best story).
#include "bump_arena.hpp"
#include "pool_allocator.hpp"
#include "free_list_allocator.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

using alloclab::BumpArena;
using alloclab::FreeListAllocator;
using alloclab::PoolAllocator;
using Clock = std::chrono::steady_clock;

namespace {

double ms_since(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

void report(const char* label, double ms, long ops) {
    double ops_per_sec = ops / (ms / 1000.0);
    std::printf("  %-28s %10.2f ms   %14.0f ops/sec\n", label, ms, ops_per_sec);
}

struct Small {
    long a, b, c, d; // 32 bytes
};

// ---------------------------------------------------------------------
// Workload 1: fixed-size alloc-then-immediately-free churn.
// Best case for a pool allocator (O(1) alloc AND free, no search).
// ---------------------------------------------------------------------
void bench_fixed_churn(long n) {
    std::printf("\n[1] fixed-size (32B) alloc+free churn, x%ld\n", n);
    // Every allocation is touched and folded into `checksum`, which is
    // printed at the end. Without this, an alloc/free pair whose pointer
    // is never used has NO externally observable effect, and both GCC and
    // Clang will happily delete the entire loop at -O2 — which is exactly
    // what happened here on the first pass (malloc/free, new/delete, and
    // PoolAllocator all reported ~0.00ms, i.e. "infinitely fast," because
    // there was nothing left to time). A benchmark that measures nothing
    // is worse than no benchmark — it looks credible while being wrong.
    volatile long checksum = 0;

    {
        auto start = Clock::now();
        for (long i = 0; i < n; ++i) {
            auto* p = static_cast<Small*>(std::malloc(sizeof(Small)));
            p->a = i;
            checksum += p->a;
            std::free(p);
        }
        report("malloc/free", ms_since(start), n);
    }
    {
        auto start = Clock::now();
        for (long i = 0; i < n; ++i) {
            auto* p = new Small;
            p->a = i;
            checksum += p->a;
            delete p;
        }
        report("new/delete", ms_since(start), n);
    }
    {
        PoolAllocator<Small> pool(64); // small pool: this workload never holds more than 1 live
        auto start = Clock::now();
        for (long i = 0; i < n; ++i) {
            Small* p = pool.allocate();
            p->a = i;
            checksum += p->a;
            pool.deallocate(p);
        }
        report("PoolAllocator", ms_since(start), n);
    }
    {
        FreeListAllocator fl(1 << 16);
        auto start = Clock::now();
        for (long i = 0; i < n; ++i) {
            auto* p = static_cast<Small*>(fl.allocate(sizeof(Small)));
            p->a = i;
            checksum += p->a;
            fl.deallocate(p);
        }
        report("FreeListAllocator", ms_since(start), n);
    }

    std::printf("  (checksum %ld — printed so the compiler can't optimize the loops away)\n", checksum);
}

// ---------------------------------------------------------------------
// Workload 2: variable-size allocation with a live working set (allocate
// a random size, occasionally free a random still-live pointer). Compares
// malloc against FreeListAllocator — the case FreeListAllocator is
// actually designed for.
// ---------------------------------------------------------------------
void bench_variable_live_set(long n) {
    std::printf("\n[2] variable-size (16-256B) alloc/free with a live working set, x%ld ops\n", n);
    constexpr std::size_t kMaxLive = 4000;

    auto run = [&](const char* label, auto alloc_fn, auto free_fn) {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> size_dist(16, 256);
        std::uniform_int_distribution<int> action_dist(0, 99);
        std::vector<void*> live;
        live.reserve(kMaxLive);

        auto start = Clock::now();
        for (long i = 0; i < n; ++i) {
            bool do_alloc = live.empty() || (live.size() < kMaxLive && action_dist(rng) < 70);
            if (do_alloc) {
                live.push_back(alloc_fn(static_cast<std::size_t>(size_dist(rng))));
            } else {
                std::uniform_int_distribution<std::size_t> pick(0, live.size() - 1);
                std::size_t idx = pick(rng);
                free_fn(live[idx]);
                live[idx] = live.back();
                live.pop_back();
            }
        }
        double ms = ms_since(start);
        for (void* p : live) free_fn(p);
        report(label, ms, n);
    };

    run("malloc/free",
        [](std::size_t sz) { return std::malloc(sz); },
        [](void* p) { std::free(p); });

    {
        FreeListAllocator fl(1 << 22); // 4MB — comfortably covers kMaxLive * ~300B worst case
        run("FreeListAllocator",
            [&](std::size_t sz) { return fl.allocate(sz); },
            [&](void* p) { fl.deallocate(p); });
    }
}

// ---------------------------------------------------------------------
// Workload 3: the arena's actual use case — allocate a batch of objects
// that all die together, then reclaim the whole batch at once. Compares
// "malloc each object, free each object individually" against "arena
// allocate each object, reset() once."
// ---------------------------------------------------------------------
void bench_arena_reset_pattern(long batches, long per_batch) {
    std::printf("\n[3] batch-allocate-then-free-all-at-once, %ld batches x %ld objects\n", batches, per_batch);

    volatile long checksum = 0;

    {
        auto start = Clock::now();
        std::vector<Small*> batch;
        batch.reserve(static_cast<std::size_t>(per_batch));
        for (long b = 0; b < batches; ++b) {
            batch.clear();
            for (long i = 0; i < per_batch; ++i) {
                auto* p = static_cast<Small*>(std::malloc(sizeof(Small)));
                p->a = i;
                batch.push_back(p);
            }
            for (Small* p : batch) { checksum += p->a; std::free(p); }
        }
        report("malloc + individual free", ms_since(start), batches * per_batch);
    }
    {
        BumpArena arena(static_cast<std::size_t>(per_batch) * sizeof(Small) + 4096);
        auto start = Clock::now();
        for (long b = 0; b < batches; ++b) {
            for (long i = 0; i < per_batch; ++i) {
                auto* p = static_cast<Small*>(arena.allocate(sizeof(Small), alignof(Small)));
                p->a = i;
                checksum += p->a;
            }
            arena.reset();
        }
        report("BumpArena + reset()", ms_since(start), batches * per_batch);
    }
    std::printf("  (checksum %ld)\n", checksum);
}

// ---------------------------------------------------------------------
// Workload 4: build a large singly-linked list, then sum it. Compares
// malloc-per-node against a PoolAllocator-per-node, measuring BUILD time
// and TRAVERSAL time separately — the interesting number here is
// traversal, where memory layout (cache locality), not allocator speed,
// is what's actually being measured.
// ---------------------------------------------------------------------
struct Node {
    long value;
    Node* next;
};

void bench_linked_list(long count) {
    std::printf("\n[4] build + traverse a %ld-node linked list\n", count);

    {
        auto build_start = Clock::now();
        Node* head = nullptr;
        for (long i = 0; i < count; ++i) {
            auto* n = new Node{i, head};
            head = n;
        }
        double build_ms = ms_since(build_start);

        auto trav_start = Clock::now();
        volatile long sum = 0;
        for (Node* n = head; n != nullptr; n = n->next) sum += n->value;
        double trav_ms = ms_since(trav_start);

        std::printf("  %-28s build %8.2f ms   traverse %8.2f ms\n", "new (scattered)", build_ms, trav_ms);

        for (Node* n = head; n != nullptr;) {
            Node* next = n->next;
            delete n;
            n = next;
        }
    }
    {
        PoolAllocator<Node> pool(static_cast<std::size_t>(count));
        auto build_start = Clock::now();
        Node* head = nullptr;
        for (long i = 0; i < count; ++i) {
            Node* n = pool.construct(Node{i, head});
            head = n;
        }
        double build_ms = ms_since(build_start);

        auto trav_start = Clock::now();
        volatile long sum = 0;
        for (Node* n = head; n != nullptr; n = n->next) sum += n->value;
        double trav_ms = ms_since(trav_start);

        std::printf("  %-28s build %8.2f ms   traverse %8.2f ms\n", "PoolAllocator (contiguous)", build_ms, trav_ms);
    }
}

} // namespace

int main() {
    std::printf("cpp-allocators benchmarks\n");
    std::printf("==========================\n");

    bench_fixed_churn(2'000'000);
    bench_variable_live_set(300'000);
    bench_arena_reset_pattern(200, 5000);
    bench_linked_list(1'000'000);

    std::printf("\ndone.\n");
    return 0;
}
