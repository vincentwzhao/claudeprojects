// align.hpp — the alignment arithmetic every allocator in this repo shares.
#pragma once

#include <cstddef>
#include <cstdint>

namespace alloclab {

// Rounds `addr` up to the next multiple of `alignment`. `alignment` must be
// a power of two (true of every alignment requirement in C++ — alignof(T)
// is always a power of two). This is the same trick malloc implementations
// use: for a power-of-two alignment, `(addr + a - 1) & ~(a - 1)` clears the
// low bits instead of doing a division/modulo.
constexpr std::uintptr_t align_up(std::uintptr_t addr, std::size_t alignment) {
    return (addr + alignment - 1) & ~(static_cast<std::uintptr_t>(alignment) - 1);
}

inline void* align_up(void* ptr, std::size_t alignment) {
    return reinterpret_cast<void*>(
        align_up(reinterpret_cast<std::uintptr_t>(ptr), alignment));
}

inline bool is_power_of_two(std::size_t x) {
    return x != 0 && (x & (x - 1)) == 0;
}

} // namespace alloclab
