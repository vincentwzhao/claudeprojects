#pragma once

#include <cstddef>
#include <cstdlib>
#include <new>
#include <utility>

namespace tensor {

// Owns a 64-byte aligned heap buffer of T. 64 bytes covers both AVX2
// (32-byte) and AVX-512 (64-byte) load/store alignment, and matches
// typical cache line size, so a row can never straddle a cache line
// unnecessarily at its start.
template <typename T>
class AlignedBuffer {
 public:
  static constexpr std::size_t kAlignment = 64;

  AlignedBuffer() = default;

  explicit AlignedBuffer(std::size_t count) : count_(count) {
    if (count_ == 0) return;
    std::size_t bytes = count_ * sizeof(T);
    // aligned_alloc requires size to be a multiple of alignment.
    bytes = (bytes + kAlignment - 1) / kAlignment * kAlignment;
    void* raw = std::aligned_alloc(kAlignment, bytes);
    if (!raw) throw std::bad_alloc();
    data_ = static_cast<T*>(raw);
  }

  ~AlignedBuffer() { std::free(data_); }

  AlignedBuffer(const AlignedBuffer&) = delete;
  AlignedBuffer& operator=(const AlignedBuffer&) = delete;

  AlignedBuffer(AlignedBuffer&& other) noexcept
      : data_(std::exchange(other.data_, nullptr)),
        count_(std::exchange(other.count_, 0)) {}

  AlignedBuffer& operator=(AlignedBuffer&& other) noexcept {
    if (this != &other) {
      std::free(data_);
      data_ = std::exchange(other.data_, nullptr);
      count_ = std::exchange(other.count_, 0);
    }
    return *this;
  }

  T* get() noexcept { return data_; }
  const T* get() const noexcept { return data_; }
  std::size_t size() const noexcept { return count_; }

 private:
  T* data_ = nullptr;
  std::size_t count_ = 0;
};

}  // namespace tensor
