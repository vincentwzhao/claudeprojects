// stl_adapter.hpp — wraps one of this repo's allocators to satisfy the C++
// standard library's Allocator named requirements, so it can be plugged
// into std::vector, std::deque, std::basic_string, etc. as the second
// template argument.
//
// This is deliberately a *view*, not an owner: StlAllocator holds a
// pointer to a Backend the caller constructed and keeps alive — it never
// constructs or destroys the backend itself. That mirrors how std::pmr
// allocators work (a std::pmr::vector doesn't own its memory_resource
// either), and it's the only sane design here: the backend's lifetime
// (and therefore reset()/capacity decisions) is a decision the CALLER
// makes, not something an allocator adapter should silently take over.
//
// Works with BumpArena and FreeListAllocator (both expose
// allocate(size_t)/deallocate(void*)). PoolAllocator is intentionally NOT
// wrapped here — it hands out fixed-size, non-contiguous blocks, which
// can't satisfy a container's request for N contiguous T's in one call
// the way vector's growth does. See docs/DESIGN.md for why.
#pragma once

#include <cstddef>
#include <memory>

namespace alloclab {

template <typename T, typename Backend>
class StlAllocator {
public:
    using value_type = T;

    explicit StlAllocator(Backend& backend) noexcept : backend_(&backend) {}

    // Lets containers rebind to allocate their internal node/element types
    // (e.g. std::vector<T, StlAllocator<T,B>> constructing a
    // StlAllocator<U,B> for some internal purpose) while sharing the same
    // backend.
    template <typename U>
    StlAllocator(const StlAllocator<U, Backend>& other) noexcept : backend_(other.backend_) {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(backend_->allocate(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t) noexcept {
        backend_->deallocate(p);
    }

    template <typename U, typename B>
    friend class StlAllocator;

    template <typename U1, typename U2, typename B>
    friend bool operator==(const StlAllocator<U1, B>&, const StlAllocator<U2, B>&) noexcept;

private:
    Backend* backend_;
};

template <typename T, typename U, typename Backend>
bool operator==(const StlAllocator<T, Backend>& a, const StlAllocator<U, Backend>& b) noexcept {
    return a.backend_ == b.backend_;
}

template <typename T, typename U, typename Backend>
bool operator!=(const StlAllocator<T, Backend>& a, const StlAllocator<U, Backend>& b) noexcept {
    return !(a == b);
}

} // namespace alloclab
