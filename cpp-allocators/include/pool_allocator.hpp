// pool_allocator.hpp — a fixed-size free-list pool allocator.
//
// All blocks are the same size (sized for T), so unlike a general-purpose
// allocator there's never a "find a big enough hole" search and never any
// fragmentation to worry about: every freed block is immediately reusable
// by the next allocation, in O(1).
//
// The free list is INTRUSIVE: a freed block's own first bytes are reused
// to store the "next free block" pointer. That's legal exactly because the
// block is free — nothing else is using those bytes — and it means the
// free list costs zero extra memory beyond the pool itself. This is the
// same trick most production fixed-size pool allocators use (e.g. small
// object allocators in game engines).
#pragma once

#include "align.hpp"

#include <cstddef>
#include <new>
#include <utility>

namespace alloclab {

template <typename T>
class PoolAllocator {
public:
    explicit PoolAllocator(std::size_t capacity_objects)
        : capacity_(capacity_objects) {
        alignment_ = alignof(T) > alignof(void*) ? alignof(T) : alignof(void*);
        std::size_t min_block = sizeof(T) > sizeof(void*) ? sizeof(T) : sizeof(void*);
        block_size_ = align_up(min_block, alignment_);

        buffer_ = static_cast<std::byte*>(::operator new(
            block_size_ * capacity_, std::align_val_t(alignment_)));

        // Thread every block onto the free list up front. Order doesn't
        // matter for correctness — LIFO reuse is simplest to build here.
        free_head_ = nullptr;
        for (std::size_t i = 0; i < capacity_; ++i) {
            auto* node = reinterpret_cast<FreeNode*>(buffer_ + i * block_size_);
            node->next = free_head_;
            free_head_ = node;
        }
    }

    ~PoolAllocator() {
        ::operator delete(buffer_, std::align_val_t(alignment_));
    }

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;
    PoolAllocator(PoolAllocator&&) = delete;
    PoolAllocator& operator=(PoolAllocator&&) = delete;

    // Raw slot allocation (uninitialized memory sized/aligned for T).
    // Throws std::bad_alloc when the pool is exhausted — no dynamic
    // growth here, by design: fixed capacity is what makes this allocator
    // predictable (no surprise heap growth mid-hot-loop).
    T* allocate() {
        if (free_head_ == nullptr) {
            throw std::bad_alloc();
        }
        FreeNode* node = free_head_;
        free_head_ = free_head_->next;
        ++used_;
        return reinterpret_cast<T*>(node);
    }

    void deallocate(T* ptr) {
        auto* node = reinterpret_cast<FreeNode*>(ptr);
        node->next = free_head_;
        free_head_ = node;
        --used_;
    }

    template <typename... Args>
    T* construct(Args&&... args) {
        T* mem = allocate();
        return ::new (static_cast<void*>(mem)) T(std::forward<Args>(args)...);
    }

    void destroy(T* obj) {
        obj->~T();
        deallocate(obj);
    }

    std::size_t capacity() const { return capacity_; }
    std::size_t used() const { return used_; }

private:
    struct FreeNode {
        FreeNode* next;
    };

    std::byte* buffer_;
    std::size_t block_size_;
    std::size_t alignment_;
    std::size_t capacity_;
    std::size_t used_ = 0;
    FreeNode* free_head_;
};

} // namespace alloclab
