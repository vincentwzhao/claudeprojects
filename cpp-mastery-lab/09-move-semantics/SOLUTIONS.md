# 09 — Move semantics: solutions

```cpp
IntArray(IntArray&& other) noexcept
    : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;
    other.size_ = 0;
}

IntArray& operator=(IntArray&& other) noexcept {
    if (this != &other) {
        delete[] data_;
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

IntArray identity(IntArray arr) {
    return arr;
}
```

Notes:
- The pattern is identical in both move functions: copy the source's
  pointer/size fields, then null out the source. The only difference is
  move assignment must first release whatever *this* already owned
  (`delete[] data_`), since unlike the constructor, `*this` already has a
  buffer that would otherwise leak.
- `identity(std::move(c))` moves `c`'s buffer into the parameter `arr`
  (pass-by-value + `std::move` at the call site = move-construct the
  parameter). Returning `arr` by value then either moves again or is
  elided entirely by the compiler (NRVO) — either way, no deep copy of the
  underlying `int` buffer happens anywhere in this call chain.
