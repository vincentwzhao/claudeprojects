# 05 — Structs & classes: solutions

```cpp
struct Rect {
    int width, height;
    Rect(int w, int h) : width(w), height(h) {}
    int area() const { return width * height; }
};

class IntBuffer {
    int* data_ = nullptr;
    int size_ = 0;
public:
    explicit IntBuffer(int size) : size_(size) {
        data_ = new int[size_];
        for (int i = 0; i < size_; ++i) data_[i] = 0;
    }

    ~IntBuffer() { delete[] data_; }

    IntBuffer(const IntBuffer& other) : size_(other.size_) {
        data_ = new int[size_];
        for (int i = 0; i < size_; ++i) data_[i] = other.data_[i];
    }

    IntBuffer& operator=(const IntBuffer& other) {
        if (this == &other) return *this;
        delete[] data_;
        size_ = other.size_;
        data_ = new int[size_];
        for (int i = 0; i < size_; ++i) data_[i] = other.data_[i];
        return *this;
    }

    int& at(int i) { return data_[i]; }
    int size() const { return size_; }
};

struct Dog : Animal {
    const char* sound() const override { return "Woof"; }
};
```

Notes:
- The self-assignment check (`if (this == &other) return *this;`) matters:
  without it, `a = a;` would `delete[]` the buffer and then try to read
  from it via `other.data_`, which now points at freed memory.
- This whole class is the textbook argument for `08-raii`: wrapping
  `data_` in a `std::unique_ptr<int[]>` (or just using `std::vector<int>`)
  makes the destructor, copy constructor, and copy assignment all
  auto-generated correctly — Rule of Zero instead of Rule of Three.
