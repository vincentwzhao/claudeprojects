// 02-references/exercises.cpp
#include "test.hpp"
#include <string>
#include <vector>

// TODO 1: swap two ints using references this time (compare to 01-pointers).
void swap_via_refs(int& a, int& b) {
    // your code here
}

// TODO 2: return a reference to whichever of a, b is larger, so the caller
// can both read AND mutate the winner through the returned reference.
int& max_via_ref(int& a, int& b) {
    // your code here — replace with a real return
    static int dummy = 0;
    return dummy;
}

// TODO 3: append " (processed)" to s, in place, via reference — don't return
// a new string.
void mark_processed(std::string& s) {
    // your code here
}

// TODO 4: given a vector of ints passed by reference, double every element
// in place.
void double_all(std::vector<int>& v) {
    // your code here
}

int main() {
    int a = 3, b = 7;
    swap_via_refs(a, b);
    CHECK_EQ(a, 7);
    CHECK_EQ(b, 3);

    int x = 5, y = 9;
    int& m = max_via_ref(x, y);
    CHECK_EQ(m, 9);
    m = 100;              // mutate through the reference
    CHECK_EQ(y, 100);     // y itself should have changed

    std::string s = "task";
    mark_processed(s);
    CHECK_EQ(s, std::string("task (processed)"));

    std::vector<int> v = {1, 2, 3};
    double_all(v);
    CHECK_EQ(v[0], 2);
    CHECK_EQ(v[1], 4);
    CHECK_EQ(v[2], 6);

    TEST_SUMMARY();
}
