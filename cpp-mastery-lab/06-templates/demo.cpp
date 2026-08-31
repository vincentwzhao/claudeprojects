// 06-templates/demo.cpp
#include <cstdio>
#include <type_traits>

// -- function template --
template <typename T>
T max_of(T a, T b) { return a > b ? a : b; }

// -- class template --
template <typename T>
class Box {
    T value_;
public:
    explicit Box(T v) : value_(v) {}
    T& get() { return value_; }
    const T& get() const { return value_; }
};

// -- non-type template parameter --
template <typename T, int N>
struct FixedArray {
    T data[N];
    constexpr int size() const { return N; }
};

// -- template specialization: a generic version plus a special-cased one --
template <typename T>
void describe(T value) {
    printf("  generic describe: (unknown type)\n");
}
template <>
void describe<int>(int value) {
    printf("  describe<int>: %d\n", value);
}
template <>
void describe<const char*>(const char* value) {
    printf("  describe<const char*>: \"%s\"\n", value);
}

// -- if constexpr: compile-time branch, C++17 --
template <typename T>
void print_kind(T value) {
    if constexpr (std::is_integral_v<T>) {
        printf("  %d is an integral type\n", (int)value);
    } else if constexpr (std::is_floating_point_v<T>) {
        printf("  %f is a floating point type\n", (double)value);
    } else {
        printf("  (non-numeric type)\n");
    }
}

// -- static_assert --
template <typename T>
struct MustBeSmall {
    static_assert(sizeof(T) <= 8, "T must fit in 8 bytes");
    T value;
};

int main() {
    printf("-- function template deduction --\n");
    printf("  max_of(3, 5) = %d\n", max_of(3, 5));
    printf("  max_of(3.5, 2.1) = %.1f\n", max_of(3.5, 2.1));

    printf("\n-- class template --\n");
    Box<int> bi(42);
    Box<double> bd(3.14);
    printf("  bi.get()=%d bd.get()=%.2f\n", bi.get(), bd.get());

    printf("\n-- non-type template parameter --\n");
    FixedArray<int, 5> fa;
    printf("  FixedArray<int,5>.size() = %d\n", fa.size());

    printf("\n-- explicit specialization --\n");
    describe(7);
    describe("hello");
    describe(3.14);   // falls through to the generic version

    printf("\n-- if constexpr --\n");
    print_kind(5);
    print_kind(2.5);

    printf("\n-- static_assert (compiles only because int fits in 8 bytes) --\n");
    MustBeSmall<int> m{10};
    printf("  m.value = %d\n", m.value);

    return 0;
}
