// 05-structs-and-classes/demo.cpp
#include <cstdio>
#include <cstring>

struct Point {  // struct: public by default, plain data
    int x, y;
    Point(int x_, int y_) : x(x_), y(y_) {}   // member initializer list
};

// Rule-of-three example: this class owns a raw resource (a C string), so it
// must define destructor, copy constructor, and copy assignment together.
class OwningString {
    char* data_;
    size_t len_;

public:
    explicit OwningString(const char* s) {
        len_ = strlen(s);
        data_ = new char[len_ + 1];
        memcpy(data_, s, len_ + 1);
        printf("  OwningString(\"%s\") constructed\n", data_);
    }

    // copy constructor: DEEP copy, not a pointer copy
    OwningString(const OwningString& other) : len_(other.len_) {
        data_ = new char[len_ + 1];
        memcpy(data_, other.data_, len_ + 1);
        printf("  OwningString copy-constructed (\"%s\")\n", data_);
    }

    // copy assignment
    OwningString& operator=(const OwningString& other) {
        if (this == &other) return *this;
        delete[] data_;
        len_ = other.len_;
        data_ = new char[len_ + 1];
        memcpy(data_, other.data_, len_ + 1);
        return *this;
    }

    ~OwningString() {
        printf("  ~OwningString(\"%s\") destroyed\n", data_);
        delete[] data_;
    }

    const char* c_str() const { return data_; }
};

void deep_copy_demo() {
    OwningString a("hello");
    OwningString b = a;                 // copy constructor: independent buffer
    printf("  a=\"%s\" b=\"%s\" (independent copies)\n", a.c_str(), b.c_str());
}   // both destructors run here, each freeing its OWN buffer — no double free

// Polymorphism + the mandatory virtual destructor.
struct Shape {
    virtual double area() const = 0;
    virtual ~Shape() { printf("  ~Shape()\n"); }
};
struct Circle : Shape {
    double r;
    explicit Circle(double r_) : r(r_) {}
    double area() const override { return 3.14159 * r * r; }
    ~Circle() override { printf("  ~Circle()\n"); }
};

void polymorphism_demo() {
    Shape* s = new Circle(2.0);
    printf("  area = %.4f\n", s->area());   // virtual dispatch -> Circle::area
    delete s;   // virtual ~Shape() ensures ~Circle() runs too, then ~Shape()
}

int main() {
    printf("-- struct with member initializer list --\n");
    Point p(3, 4);
    printf("  p = (%d, %d)\n", p.x, p.y);

    printf("\n-- rule of three: deep copy --\n");
    deep_copy_demo();

    printf("\n-- virtual dispatch + virtual destructor --\n");
    polymorphism_demo();

    return 0;
}
