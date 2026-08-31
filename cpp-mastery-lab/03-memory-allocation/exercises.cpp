// 03-memory-allocation/exercises.cpp
#include "test.hpp"
#include <cstdlib>

// TODO 1: allocate an int array of `n` elements on the heap with new[],
// fill it 0..n-1, return the pointer. Caller owns it (must delete[]).
int* make_range(int n) {
    // your code here
    return nullptr;
}

// TODO 2: free what make_range allocated. (Trivial, but forces you to think
// about which delete form matches new[].)
void free_range(int* p) {
    // your code here
}

// TODO 3: deep-copy a heap int array of length n into a NEW heap array,
// so mutating the copy never affects the original.
int* deep_copy(const int* src, int n) {
    // your code here
    return nullptr;
}

// TODO 4: implement a minimal malloc-based "resize": allocate a new block
// of new_n ints, copy min(old_n,new_n) elements from p, free p, return the
// new pointer. (This is basically what realloc does — don't call realloc,
// implement it with malloc/free so the mechanics are explicit.)
int* manual_resize(int* p, int old_n, int new_n) {
    // your code here
    return nullptr;
}

int main() {
    int* a = make_range(5);
    CHECK(a != nullptr);
    CHECK_EQ(a[0], 0);
    CHECK_EQ(a[4], 4);

    int* b = deep_copy(a, 5);
    b[0] = 999;
    CHECK_EQ(a[0], 0);       // original untouched
    CHECK_EQ(b[0], 999);

    free_range(a);
    free_range(b);

    int* c = static_cast<int*>(malloc(sizeof(int) * 3));
    c[0] = 1; c[1] = 2; c[2] = 3;
    c = manual_resize(c, 3, 5);
    CHECK_EQ(c[0], 1);
    CHECK_EQ(c[1], 2);
    CHECK_EQ(c[2], 3);
    free(c);

    TEST_SUMMARY();
}
