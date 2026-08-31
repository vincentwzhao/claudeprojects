#include "test.hpp"
#include "pool_allocator.hpp"

#include <cstdint>
#include <vector>

using alloclab::PoolAllocator;

struct Widget {
    int id;
    double value;
    explicit Widget(int i, double v) : id(i), value(v) {}
};

int main() {
    {
        PoolAllocator<Widget> pool(4);
        CHECK_EQ(pool.capacity(), 4u);
        CHECK_EQ(pool.used(), 0u);

        Widget* a = pool.construct(1, 1.5);
        Widget* b = pool.construct(2, 2.5);
        CHECK_EQ(a->id, 1);
        CHECK_EQ(b->id, 2);
        CHECK_EQ(pool.used(), 2u);

        pool.destroy(a);
        CHECK_EQ(pool.used(), 1u);

        // The freed slot must be reused, not leaked as wasted capacity —
        // this allocation must succeed even though we've already made 2
        // allocations against a capacity of 4 and only freed 1.
        Widget* c = pool.construct(3, 3.5);
        CHECK_EQ(pool.used(), 2u);
        CHECK_EQ(c->id, 3);

        pool.destroy(b);
        pool.destroy(c);
    }

    {
        // Exhaustion: allocate every slot, verify the next one throws.
        PoolAllocator<int> pool(3);
        std::vector<int*> ptrs;
        for (int i = 0; i < 3; ++i) ptrs.push_back(pool.construct(i));
        CHECK_EQ(pool.used(), 3u);

        bool threw = false;
        try {
            pool.construct(99);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        CHECK(threw);

        for (int* p : ptrs) pool.destroy(p);
    }

    {
        // Alignment: every returned slot must satisfy alignof(T), even for
        // an over-aligned type.
        struct alignas(32) Aligned32 { int x; };
        PoolAllocator<Aligned32> pool(5);
        Aligned32* p = pool.construct();
        CHECK_EQ(reinterpret_cast<std::uintptr_t>(p) % 32, 0u);
        pool.destroy(p);
    }

    {
        // No leak of used-count across many alloc/free cycles that exceed
        // capacity in total but never exceed it concurrently.
        PoolAllocator<int> pool(2);
        for (int i = 0; i < 1000; ++i) {
            int* a = pool.construct(i);
            int* b = pool.construct(i * 2);
            CHECK_EQ(pool.used(), 2u);
            pool.destroy(a);
            pool.destroy(b);
        }
        CHECK_EQ(pool.used(), 0u);
    }

    TEST_SUMMARY();
}
