// 09-move-semantics/exercises.cpp
#include "test.hpp"
#include <utility>

// A resource-owning class you'll add move support to.
class IntArray {
    int* data_ = nullptr;
    int size_ = 0;

public:
    explicit IntArray(int size) : size_(size) {
        data_ = new int[size_];
        for (int i = 0; i < size_; ++i) data_[i] = i;
    }

    ~IntArray() { delete[] data_; }

    // copy constructor already given (deep copy) — leave as is
    IntArray(const IntArray& other) : size_(other.size_) {
        data_ = new int[size_];
        for (int i = 0; i < size_; ++i) data_[i] = other.data_[i];
    }

    // TODO 1: move constructor. Steal other's buffer; leave `other` with
    // data_=nullptr, size_=0 so its destructor is a safe no-op.
    IntArray(IntArray&& other) noexcept {
        // your code here
    }

    // TODO 2: move assignment. Release our own buffer first, then steal.
    // Guard against self-move-assignment.
    IntArray& operator=(IntArray&& other) noexcept {
        // your code here
        return *this;
    }

    int size() const { return size_; }
    int at(int i) const { return data_[i]; }
    bool is_empty_state() const { return data_ == nullptr && size_ == 0; }
};

// TODO 3: write a function that takes an IntArray by value (forcing a
// move when called with std::move, or a copy otherwise) and returns it —
// this exercises understanding of when moves happen automatically.
IntArray identity(IntArray arr) {
    // your code here — just return arr (return of a local by value is
    // itself typically elided or moved, not copied)
    return arr;
}

int main() {
    IntArray a(5);
    CHECK_EQ(a.size(), 5);
    CHECK_EQ(a.at(4), 4);

    IntArray b = std::move(a);          // should invoke move constructor
    CHECK_EQ(b.size(), 5);
    CHECK_EQ(b.at(4), 4);
    CHECK(a.is_empty_state());          // a should now be empty (moved-from)

    IntArray c(2);
    c = std::move(b);                    // should invoke move assignment
    CHECK_EQ(c.size(), 5);
    CHECK_EQ(c.at(4), 4);
    CHECK(b.is_empty_state());

    IntArray d = identity(std::move(c));
    CHECK_EQ(d.size(), 5);
    CHECK_EQ(d.at(0), 0);

    TEST_SUMMARY();
}
