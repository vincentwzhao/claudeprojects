#include "test.hpp"
#include "free_list_allocator.hpp"

#include <cstring>
#include <vector>

using alloclab::FreeListAllocator;

int main() {
    {
        FreeListAllocator alloc(4096);
        void* a = alloc.allocate(64);
        void* b = alloc.allocate(128);
        CHECK(a != nullptr);
        CHECK(b != nullptr);
        CHECK(a != b);

        std::memset(a, 0xAB, 64);
        std::memset(b, 0xCD, 128);
        CHECK_EQ(static_cast<unsigned char*>(a)[0], 0xAB);
        CHECK_EQ(static_cast<unsigned char*>(b)[0], 0xCD);

        alloc.deallocate(a);
        alloc.deallocate(b);
    }

    {
        // The core promise of a general-purpose allocator: allocate,
        // free, and reallocate in an order that would fragment a naive
        // allocator, and verify a big-enough later allocation still fits
        // because coalescing merged the freed neighbors back together.
        //
        // The arena is sized to exactly fit 3 blocks of 208 payload bytes
        // each (208 + 16-byte header + 16-byte footer = 240; 3*240=720),
        // so there's no leftover "remainder" free block to muddy the
        // measurements below — every byte of free space we see comes from
        // blocks we explicitly freed.
        FreeListAllocator alloc(720);
        void* a = alloc.allocate(208);
        void* b = alloc.allocate(208);
        void* c = alloc.allocate(208);
        CHECK_EQ(alloc.free_block_count(), 0u);   // fully consumed, no remainder block

        alloc.deallocate(b);   // freeing the middle block alone: no merge partner yet
        // (a and c are still allocated, so b's neighbors aren't free — no coalescing possible yet)
        CHECK_EQ(alloc.largest_free_block(), 208u);

        alloc.deallocate(a);   // now a and b are adjacent frees -> should coalesce
        CHECK_EQ(alloc.largest_free_block(), 208u * 2 + 32u);   // merged payload + a's swallowed header/footer

        alloc.deallocate(c);   // c should now merge into the (a+b) block too
        CHECK_EQ(alloc.largest_free_block(), 208u * 3 + 32u * 2);   // == whole arena's payload again (720-32)

        // A single allocation spanning roughly all three original blocks'
        // combined payload must now succeed — proof the coalesced region
        // is one contiguous usable block, not three separate small ones.
        void* big = alloc.allocate(600);
        CHECK(big != nullptr);
        alloc.deallocate(big);
    }

    {
        // Splitting: a large free block serving a small request should
        // leave a usable remainder behind, not consume the whole block.
        FreeListAllocator alloc(4096);
        void* small = alloc.allocate(32);
        std::size_t remaining_after_small = alloc.largest_free_block();
        CHECK(remaining_after_small > 4096 - 200); // most of the arena is still free
        alloc.deallocate(small);
    }

    {
        // Exhaustion: request more than the whole arena can hold.
        FreeListAllocator alloc(256);
        bool threw = false;
        try {
            alloc.allocate(10000);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        CHECK(threw);
    }

    {
        // Many interleaved alloc/free cycles shouldn't corrupt bookkeeping
        // — every pointer written to must still read back correctly.
        FreeListAllocator alloc(1 << 16);
        std::vector<std::pair<unsigned char*, unsigned char>> live;
        for (int round = 0; round < 500; ++round) {
            auto* p = static_cast<unsigned char*>(alloc.allocate(16 + (round % 5) * 8));
            unsigned char tag = static_cast<unsigned char>(round % 256);
            *p = tag;
            live.emplace_back(p, tag);
            if (live.size() > 20) {
                auto [ptr, expected] = live.front();
                CHECK_EQ(*ptr, expected);
                alloc.deallocate(ptr);
                live.erase(live.begin());
            }
        }
        for (auto [ptr, expected] : live) {
            CHECK_EQ(*ptr, expected);
            alloc.deallocate(ptr);
        }
    }

    TEST_SUMMARY();
}
