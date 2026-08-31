// 05-structs-and-classes/exercises.cpp
#include "test.hpp"
#include <cstring>

// TODO 1: complete this struct's constructor using a member initializer
// list (not assignment in the body).
struct Rect {
    int width, height;
    Rect(int w, int h) /* : ... */ {
        // your code here — should NOT assign in the body
    }
    int area() const { return width * height; }
};

// TODO 2: this class owns a heap buffer. Implement the Rule of Three:
// destructor, copy constructor, copy assignment — all must deep-copy so
// that `IntBuffer b = a;` gives b an independent buffer.
class IntBuffer {
    int* data_ = nullptr;
    int size_ = 0;

public:
    explicit IntBuffer(int size) : size_(size) {
        data_ = new int[size_];
        for (int i = 0; i < size_; ++i) data_[i] = 0;
    }

    // TODO: destructor
    ~IntBuffer() {
        // your code here
    }

    // TODO: copy constructor (deep copy)
    IntBuffer(const IntBuffer& other) {
        // your code here
    }

    // TODO: copy assignment (deep copy, handle self-assignment)
    IntBuffer& operator=(const IntBuffer& other) {
        // your code here
        return *this;
    }

    int& at(int i) { return data_[i]; }
    int size() const { return size_; }
};

// TODO 3: abstract base with a pure virtual + virtual destructor, and a
// derived class that overrides it. Fill in the missing pieces.
struct Animal {
    virtual const char* sound() const = 0;
    virtual ~Animal() = default;
};

struct Dog : Animal {
    // your code here — override sound() to return "Woof"
    const char* sound() const override { return ""; }
};

int main() {
    Rect r(4, 5);
    CHECK_EQ(r.width, 4);
    CHECK_EQ(r.height, 5);
    CHECK_EQ(r.area(), 20);

    IntBuffer a(3);
    a.at(0) = 1; a.at(1) = 2; a.at(2) = 3;
    IntBuffer b = a;             // copy constructor
    b.at(0) = 999;
    CHECK_EQ(a.at(0), 1);        // a must be untouched (deep copy)
    CHECK_EQ(b.at(0), 999);

    IntBuffer c(1);
    c = a;                        // copy assignment
    CHECK_EQ(c.at(0), 1);
    CHECK_EQ(c.at(1), 2);
    CHECK_EQ(c.size(), 3);

    Animal* animal = new Dog();
    CHECK_EQ(strcmp(animal->sound(), "Woof"), 0);
    delete animal;

    TEST_SUMMARY();
}
