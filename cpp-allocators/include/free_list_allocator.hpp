// free_list_allocator.hpp — a general-purpose, variable-size allocator over
// one backing buffer. This is the "I wrote my own malloc" piece: unlike
// BumpArena (can't free individual objects) or PoolAllocator (fixed size
// only), this supports arbitrary-size allocate/deallocate in any order —
// at the cost of a search on allocate and coalescing work on deallocate.
//
// Layout, boundary-tag style (the classic K&R malloc technique):
//
//   [Header][payload...][Footer][Header][payload...][Footer]...
//
// Every block — free or allocated — has a Header (size + free flag) at its
// start and a Footer (size only) at its end. The footer is what makes
// backward coalescing possible: to find the block immediately BEFORE this
// one in memory, read the footer sitting right before this block's header
// — it tells you that previous block's size, and therefore where its
// header starts, without needing any global block index.
//
// Free blocks additionally store an intrusive doubly-linked free-list
// pointer pair in their payload (only valid while free, exactly like
// PoolAllocator's free list) so allocate() only has to search actual free
// blocks, not every block in the arena.
#pragma once

#include "align.hpp"

#include <cstddef>
#include <new>
#include <stdexcept>

namespace alloclab {

class FreeListAllocator {
public:
    explicit FreeListAllocator(std::size_t capacity_bytes)
        : buffer_(static_cast<std::byte*>(::operator new(capacity_bytes))),
          capacity_(capacity_bytes) {
        if (capacity_bytes < kHeaderSize + kFooterSize + kMinPayload) {
            ::operator delete(buffer_);
            throw std::invalid_argument("FreeListAllocator: capacity too small to hold one block");
        }
        Header* first = header_at(buffer_);
        first->size = capacity_ - kHeaderSize - kFooterSize;
        first->free = true;
        write_footer(first);
        free_head_ = nullptr;
        push_free(first);
    }

    ~FreeListAllocator() { ::operator delete(buffer_); }

    FreeListAllocator(const FreeListAllocator&) = delete;
    FreeListAllocator& operator=(const FreeListAllocator&) = delete;
    FreeListAllocator(FreeListAllocator&&) = delete;
    FreeListAllocator& operator=(FreeListAllocator&&) = delete;

    // First-fit search over the free list. Splits the found block if the
    // leftover is large enough to be a useful block on its own; otherwise
    // hands over the whole block (accepting some internal fragmentation
    // rather than creating an unusably tiny free sliver).
    void* allocate(std::size_t requested) {
        std::size_t needed = align_up(requested, kAlign);
        if (needed < kMinPayload) needed = kMinPayload;

        Header* block = find_first_fit(needed);
        if (block == nullptr) throw std::bad_alloc();

        remove_free(block);

        std::size_t remainder = block->size - needed;
        if (remainder >= kHeaderSize + kFooterSize + kMinPayload) {
            split(block, needed);
        }

        block->free = false;
        write_footer(block);
        return payload_of(block);
    }

    // Marks the block free, then coalesces with whichever physical
    // neighbors (previous and/or next in memory) are also free, so
    // fragmentation doesn't accumulate across alloc/free churn.
    void deallocate(void* ptr) {
        if (ptr == nullptr) return;
        Header* block = header_from_payload(ptr);
        block->free = true;

        if (Header* next = next_physical(block); next != nullptr && next->free) {
            remove_free(next);
            block->size += kHeaderSize + kFooterSize + next->size;
        }

        if (Header* prev = prev_physical(block); prev != nullptr && prev->free) {
            remove_free(prev);
            prev->size += kHeaderSize + kFooterSize + block->size;
            block = prev;
        }

        write_footer(block);
        push_free(block);
    }

    std::size_t capacity() const { return capacity_; }

    // Sum of every free block's usable payload — for benchmarking/
    // diagnostics, to see how fragmentation evolves under churn.
    std::size_t largest_free_block() const {
        std::size_t best = 0;
        for (FreeNode* n = free_head_; n != nullptr; n = n->next) {
            Header* h = reinterpret_cast<Header*>(reinterpret_cast<std::byte*>(n) - kHeaderSize);
            if (h->size > best) best = h->size;
        }
        return best;
    }

    std::size_t free_block_count() const {
        std::size_t count = 0;
        for (FreeNode* n = free_head_; n != nullptr; n = n->next) ++count;
        return count;
    }

private:
    struct Header {
        std::size_t size; // usable payload size, excludes header/footer
        bool free;
    };
    struct Footer {
        std::size_t size;
    };
    struct FreeNode {
        FreeNode* next;
        FreeNode* prev;
    };

    static constexpr std::size_t kAlign = alignof(std::max_align_t);
    static constexpr std::size_t kHeaderSize = align_up(sizeof(Header), kAlign);
    static constexpr std::size_t kFooterSize = align_up(sizeof(Footer), kAlign);
    static constexpr std::size_t kMinPayload = align_up(sizeof(FreeNode), kAlign);

    std::byte* buffer_;
    std::size_t capacity_;
    FreeNode* free_head_;

    Header* header_at(std::byte* p) const { return reinterpret_cast<Header*>(p); }

    std::byte* payload_of(Header* h) const {
        return reinterpret_cast<std::byte*>(h) + kHeaderSize;
    }

    Header* header_from_payload(void* p) const {
        return reinterpret_cast<Header*>(static_cast<std::byte*>(p) - kHeaderSize);
    }

    Footer* footer_of(Header* h) const {
        return reinterpret_cast<Footer*>(
            reinterpret_cast<std::byte*>(h) + kHeaderSize + h->size);
    }

    void write_footer(Header* h) { footer_of(h)->size = h->size; }

    // The block immediately after `h` in memory, or nullptr if `h` is the
    // last block in the buffer.
    Header* next_physical(Header* h) const {
        std::byte* candidate =
            reinterpret_cast<std::byte*>(h) + kHeaderSize + h->size + kFooterSize;
        if (candidate >= buffer_ + capacity_) return nullptr;
        return header_at(candidate);
    }

    // The block immediately before `h` in memory, found by reading the
    // footer that sits right before `h`'s header — the boundary-tag trick.
    Header* prev_physical(Header* h) const {
        std::byte* h_bytes = reinterpret_cast<std::byte*>(h);
        if (h_bytes <= buffer_) return nullptr;
        Footer* prev_footer = reinterpret_cast<Footer*>(h_bytes - kFooterSize);
        std::byte* prev_header_bytes = h_bytes - kFooterSize - prev_footer->size - kHeaderSize;
        return header_at(prev_header_bytes);
    }

    void push_free(Header* h) {
        auto* node = reinterpret_cast<FreeNode*>(payload_of(h));
        node->prev = nullptr;
        node->next = free_head_;
        if (free_head_ != nullptr) free_head_->prev = node;
        free_head_ = node;
    }

    void remove_free(Header* h) {
        auto* node = reinterpret_cast<FreeNode*>(payload_of(h));
        if (node->prev != nullptr) node->prev->next = node->next;
        else free_head_ = node->next;
        if (node->next != nullptr) node->next->prev = node->prev;
    }

    Header* find_first_fit(std::size_t needed) const {
        for (FreeNode* n = free_head_; n != nullptr; n = n->next) {
            Header* h = reinterpret_cast<Header*>(reinterpret_cast<std::byte*>(n) - kHeaderSize);
            if (h->size >= needed) return h;
        }
        return nullptr;
    }

    // Carves `needed` bytes off the front of `block`, leaving the
    // remainder as a new free block right after it in memory.
    void split(Header* block, std::size_t needed) {
        std::size_t original_size = block->size;
        block->size = needed;
        write_footer(block);

        Header* remainder = header_at(
            reinterpret_cast<std::byte*>(block) + kHeaderSize + needed + kFooterSize);
        remainder->size = original_size - needed - kHeaderSize - kFooterSize;
        remainder->free = true;
        write_footer(remainder);
        push_free(remainder);
    }
};

} // namespace alloclab
