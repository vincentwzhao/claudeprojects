#include "test.hpp"
#include "stl_adapter.hpp"
#include "bump_arena.hpp"
#include "free_list_allocator.hpp"

#include <vector>
#include <string>

using alloclab::BumpArena;
using alloclab::FreeListAllocator;
using alloclab::StlAllocator;

int main() {
    {
        // std::vector backed by a BumpArena: growth reallocations just
        // bump the arena pointer further (wasting the old space until
        // reset()) — fine for a scratch/short-lived vector, and it proves
        // the adapter satisfies vector's allocator requirements at all.
        BumpArena arena(1 << 16);
        std::vector<int, StlAllocator<int, BumpArena>> v{StlAllocator<int, BumpArena>(arena)};
        for (int i = 0; i < 100; ++i) v.push_back(i);
        CHECK_EQ(v.size(), 100u);
        CHECK_EQ(v[0], 0);
        CHECK_EQ(v[99], 99);

        long sum = 0;
        for (int x : v) sum += x;
        CHECK_EQ(sum, 99L * 100L / 2L);
    }

    {
        // std::vector backed by FreeListAllocator: growth reallocations
        // free the old block (coalescing it back into the free list) and
        // allocate a bigger one — the allocator that's actually meant for
        // this usage pattern.
        FreeListAllocator fl(1 << 16);
        std::vector<int, StlAllocator<int, FreeListAllocator>> v{StlAllocator<int, FreeListAllocator>(fl)};
        for (int i = 0; i < 500; ++i) v.push_back(i * 2);
        CHECK_EQ(v.size(), 500u);
        CHECK_EQ(v[0], 0);
        CHECK_EQ(v[499], 998);
    }

    {
        // A non-trivial element type (std::string) to confirm constructors/
        // destructors run correctly through the adapter, not just raw ints.
        FreeListAllocator fl(1 << 16);
        std::vector<std::string, StlAllocator<std::string, FreeListAllocator>> v{
            StlAllocator<std::string, FreeListAllocator>(fl)};
        v.push_back("hello");
        v.push_back("allocator world, long enough to defeat SSO maybe");
        CHECK_EQ(v[0], std::string("hello"));
        CHECK_EQ(v.size(), 2u);
    }

    {
        // Two allocators wrapping the SAME backend must compare equal
        // (required by the standard so containers can swap/move between
        // them); two wrapping DIFFERENT backends must compare unequal.
        FreeListAllocator fl1(1024);
        FreeListAllocator fl2(1024);
        StlAllocator<int, FreeListAllocator> a1(fl1);
        StlAllocator<int, FreeListAllocator> a2(fl1);
        StlAllocator<int, FreeListAllocator> a3(fl2);
        CHECK(a1 == a2);
        CHECK(a1 != a3);
    }

    TEST_SUMMARY();
}
