// 06-templates/exercises.cpp
#include "test.hpp"
#include <string>

// TODO 1: generic min function template.
template <typename T>
T min_of(T a, T b) {
    // your code here
    return a;
}

// TODO 2: a generic Pair class template holding two values of (possibly
// different) types First and Second, with a swap() that swaps them
// (requires First == Second to actually swap in place here — keep it
// simple: just implement first()/second() accessors and a make_pair-style
// free function).
template <typename First, typename Second>
struct Pair {
    First first;
    Second second;
    Pair(First f, Second s) : first(f), second(s) {}
};

template <typename First, typename Second>
Pair<First, Second> make_pair_of(First f, Second s) {
    // your code here — construct and return a Pair<First, Second>
    return Pair<First, Second>(f, s);
}

// TODO 3: a template function `sum_all` that takes a C-style array and its
// length and returns the sum, generic over the element type T. Assume T
// supports operator+ and has a zero-constructible default (T{}).
template <typename T>
T sum_all(const T* arr, int len) {
    // your code here
    return T{};
}

// TODO 4: write a template `clamp_value(T value, T lo, T hi)` that returns
// value clamped into [lo, hi].
template <typename T>
T clamp_value(T value, T lo, T hi) {
    // your code here
    return value;
}

int main() {
    CHECK_EQ(min_of(3, 5), 3);
    CHECK_EQ(min_of(5.5, 2.2), 2.2);

    auto p = make_pair_of(std::string("age"), 30);
    CHECK_EQ(p.first, std::string("age"));
    CHECK_EQ(p.second, 30);

    int arr[] = {1, 2, 3, 4, 5};
    CHECK_EQ(sum_all(arr, 5), 15);

    double darr[] = {1.5, 2.5};
    CHECK_EQ(sum_all(darr, 2), 4.0);

    CHECK_EQ(clamp_value(15, 0, 10), 10);
    CHECK_EQ(clamp_value(-5, 0, 10), 0);
    CHECK_EQ(clamp_value(5, 0, 10), 5);

    TEST_SUMMARY();
}
