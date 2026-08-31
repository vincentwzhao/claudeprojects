// test_stress.cpp — randomized alloc/free/realloc-shaped fuzzing for all
// three allocators. Not correctness assertions in the CHECK() sense so
// much as a target for AddressSanitizer/UndefinedBehaviorSanitizer: build
// with -DSANITIZE=ON and run this binary — a heap overflow, use-after-free,
// or misaligned access anywhere in the allocator internals will show up as
// an ASan report pointing at the exact line. Clean output = clean run.
#include "test.hpp"
#include "bump_arena.hpp"
#include "pool_allocator.hpp"
#include "free_list_allocator.hpp"

#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>

using alloclab::BumpArena;
using alloclab::FreeListAllocator;
using alloclab::PoolAllocator;

namespace {

// Every live allocation gets stamped with a byte pattern derived from its
// own address, then re-checked before being freed. If any allocator ever
// hands out overlapping memory, or corrupts an adjacent live block during
// split/coalesce, this catches it even without a sanitizer attached.
void stamp(unsigned char* p, std::size_t size, unsigned char tag) {
    for (std::size_t i = 0; i < size; ++i) p[i] = static_cast<unsigned char>(tag + i);
}
bool check_stamp(const unsigned char* p, std::size_t size, unsigned char tag) {
    for (std::size_t i = 0; i < size; ++i) {
        if (p[i] != static_cast<unsigned char>(tag + i)) return false;
    }
    return true;
}

void stress_free_list(std::mt19937& rng, int iterations) {
    FreeListAllocator alloc(1 << 20);
    std::uniform_int_distribution<int> size_dist(1, 512);
    std::uniform_int_distribution<int> action_dist(0, 99);

    // Cap concurrent live allocations well under what the 1MB arena can
    // hold (average block ~288 bytes with header/footer overhead, so 2000
    // blocks is ~576KB) — otherwise a 70%-alloc bias grows the live set
    // until the arena is exhausted and allocate() throws bad_alloc, which
    // isn't the thing this test is trying to exercise.
    constexpr std::size_t kMaxLive = 2000;

    struct Live { unsigned char* ptr; std::size_t size; unsigned char tag; };
    std::vector<Live> live;

    for (int i = 0; i < iterations; ++i) {
        bool do_alloc = live.empty() || (live.size() < kMaxLive && action_dist(rng) < 70);
        if (do_alloc) {
            std::size_t size = static_cast<std::size_t>(size_dist(rng));
            auto* p = static_cast<unsigned char*>(alloc.allocate(size));
            unsigned char tag = static_cast<unsigned char>(rng() & 0xFF);
            stamp(p, size, tag);
            live.push_back({p, size, tag});
        } else {
            std::uniform_int_distribution<std::size_t> pick(0, live.size() - 1);
            std::size_t idx = pick(rng);
            CHECK(check_stamp(live[idx].ptr, live[idx].size, live[idx].tag));
            alloc.deallocate(live[idx].ptr);
            live[idx] = live.back();
            live.pop_back();
        }
    }
    for (auto& l : live) {
        CHECK(check_stamp(l.ptr, l.size, l.tag));
        alloc.deallocate(l.ptr);
    }
}

void stress_pool(std::mt19937& rng, int iterations) {
    struct Slot { std::uint64_t tag; };
    PoolAllocator<Slot> pool(256);
    std::uniform_int_distribution<int> action_dist(0, 99);
    std::vector<Slot*> live;

    for (int i = 0; i < iterations; ++i) {
        bool do_alloc = live.empty() || (live.size() < 256 && action_dist(rng) < 70);
        if (do_alloc) {
            Slot* s = pool.construct(Slot{rng()});
            live.push_back(s);
        } else {
            std::uniform_int_distribution<std::size_t> pick(0, live.size() - 1);
            std::size_t idx = pick(rng);
            pool.destroy(live[idx]);
            live[idx] = live.back();
            live.pop_back();
        }
    }
    for (Slot* s : live) pool.destroy(s);
}

void stress_arena(std::mt19937& rng, int iterations) {
    BumpArena arena(1 << 16);
    std::uniform_int_distribution<int> size_dist(1, 256);

    for (int i = 0; i < iterations; ++i) {
        std::size_t size = static_cast<std::size_t>(size_dist(rng));
        try {
            auto* p = static_cast<unsigned char*>(arena.allocate(size));
            p[0] = 0xFF;
            p[size - 1] = 0xEE;   // touch both ends of the allocation
        } catch (const std::bad_alloc&) {
            arena.reset();   // out of room: rewind and keep going
        }
    }
}

} // namespace

int main() {
    std::mt19937 rng(1337);   // fixed seed: reproducible across runs

    stress_free_list(rng, 20000);
    stress_pool(rng, 20000);
    stress_arena(rng, 20000);

    printf("stress test completed: %d checks, %d failed\n", g_tests_run, g_tests_failed);
    TEST_SUMMARY();
}
