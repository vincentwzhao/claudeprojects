// bump_arena.hpp — a bump-pointer arena allocator.
//
// Allocation is a pointer bump: O(1), no bookkeeping, no per-object free.
// The entire arena is released at once via reset() (or the destructor).
// This is the right tool when a batch of allocations shares a lifetime —
// per-request scratch memory, a single frame of a game loop, building an
// AST you'll walk once and discard. It is the WRONG tool when individual
// objects need to be freed independently; use PoolAllocator or
// FreeListAllocator for that.
//
// Design tradeoff, stated explicitly: reset() does NOT run destructors.
// The arena only knows about raw bytes, not the objects living in them.
// If you construct() non-trivially-destructible objects, you are
// responsible for destroying them yourself before reset() — the arena
// won't do it for you. This mirrors real arena allocators (e.g. protobuf's
// Arena, or typical game-engine frame allocators): the performance win
// comes precisely from NOT tracking per-object lifetime.
#pragma once

#include "align.hpp"

#include <cstddef>
#include <new>
#include <utility>

namespace alloclab {

class BumpArena {
public:
    explicit BumpArena(std::size_t capacity_bytes)
        : buffer_(static_cast<std::byte*>(::operator new(capacity_bytes))),
          capacity_(capacity_bytes),
          offset_(0) {}

    ~BumpArena() { ::operator delete(buffer_); }

    BumpArena(const BumpArena&) = delete;
    BumpArena& operator=(const BumpArena&) = delete;

    BumpArena(BumpArena&& other) noexcept
        : buffer_(other.buffer_), capacity_(other.capacity_), offset_(other.offset_) {
        other.buffer_ = nullptr;
        other.capacity_ = 0;
        other.offset_ = 0;
    }

    BumpArena& operator=(BumpArena&& other) noexcept {
        if (this != &other) {
            ::operator delete(buffer_);
            buffer_ = other.buffer_;
            capacity_ = other.capacity_;
            offset_ = other.offset_;
            other.buffer_ = nullptr;
            other.capacity_ = 0;
            other.offset_ = 0;
        }
        return *this;
    }

    // Raw allocation: `size` bytes, aligned to `alignment` (must be a power
    // of two). Throws std::bad_alloc if the arena doesn't have room.
    void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) {
        std::byte* current = buffer_ + offset_;
        std::byte* aligned = static_cast<std::byte*>(align_up(current, alignment));
        std::size_t padding = static_cast<std::size_t>(aligned - current);

        if (offset_ + padding + size > capacity_) {
            throw std::bad_alloc();
        }

        offset_ += padding + size;
        return aligned;
    }

    // Convenience: allocate + construct in place. See the class comment —
    // you own calling destroy<T>() (or the object's destructor directly)
    // before reset() if T has a non-trivial destructor.
    template <typename T, typename... Args>
    T* construct(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        return ::new (mem) T(std::forward<Args>(args)...);
    }

    template <typename T>
    void destroy(T* obj) {
        obj->~T();
    }

    // No-op by design: an arena has no notion of freeing a single
    // allocation, only rewinding the whole thing via reset(). This exists
    // so BumpArena satisfies the same allocate(size)/deallocate(ptr)
    // shape as the other allocators, which is what stl_adapter.hpp relies
    // on to wrap any of them uniformly as a C++ Allocator.
    void deallocate(void*) noexcept {}

    // Releases every allocation made since construction (or the last
    // reset()) by rewinding the bump pointer. O(1) regardless of how many
    // allocations happened — the whole point of this allocator.
    void reset() { offset_ = 0; }

    std::size_t bytes_used() const { return offset_; }
    std::size_t capacity() const { return capacity_; }

private:
    std::byte* buffer_;
    std::size_t capacity_;
    std::size_t offset_;
};

} // namespace alloclab
