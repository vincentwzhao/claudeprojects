#include "test.hpp"
#include "bump_arena.hpp"

#include <cstdint>
#include <stdexcept>

using alloclab::BumpArena;

struct Point3 {
    double x, y, z;
};

struct Tracked {
    static int live_count;
    Tracked() { ++live_count; }
    ~Tracked() { --live_count; }
};
int Tracked::live_count = 0;

int main() {
    {
        BumpArena arena(1024);
        CHECK_EQ(arena.bytes_used(), 0u);

        int* a = arena.construct<int>(42);
        int* b = arena.construct<int>(43);
        CHECK_EQ(*a, 42);
        CHECK_EQ(*b, 43);
        CHECK(reinterpret_cast<std::byte*>(b) > reinterpret_cast<std::byte*>(a));
        CHECK(arena.bytes_used() > 0u);
    }

    {
        // Alignment: every allocation must land on an address that's a
        // multiple of the requested alignment, even after odd-sized
        // allocations shift the bump pointer to an unaligned position.
        BumpArena arena(1024);
        arena.allocate(1, 1);              // deliberately misalign the pointer
        Point3* p = arena.construct<Point3>(Point3{1.0, 2.0, 3.0});
        CHECK_EQ(reinterpret_cast<std::uintptr_t>(p) % alignof(Point3), 0u);
        CHECK_EQ(p->x, 1.0);
    }

    {
        // reset() rewinds the bump pointer and lets the arena be reused —
        // but does NOT call destructors, so we destroy Tracked explicitly.
        BumpArena arena(1024);
        Tracked* t = arena.construct<Tracked>();
        CHECK_EQ(Tracked::live_count, 1);
        arena.destroy(t);
        CHECK_EQ(Tracked::live_count, 0);
        arena.reset();
        CHECK_EQ(arena.bytes_used(), 0u);

        int* reused = arena.construct<int>(7);
        CHECK_EQ(*reused, 7);
    }

    {
        // Exhaustion throws std::bad_alloc, matching `new`'s failure mode.
        BumpArena arena(8);
        bool threw = false;
        try {
            arena.allocate(1024);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        CHECK(threw);
    }

    {
        // Move semantics: moved-from arena is safely destructible and the
        // moved-to arena keeps the original's allocations valid.
        BumpArena a(64);
        int* p = a.construct<int>(99);
        BumpArena b = std::move(a);
        CHECK_EQ(*p, 99);              // b now owns the buffer p points into
        CHECK_EQ(a.bytes_used(), 0u);  // a was left empty by the move
        CHECK(b.bytes_used() > 0u);
    }

    TEST_SUMMARY();
}
