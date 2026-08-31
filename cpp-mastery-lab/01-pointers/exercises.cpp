// 01-pointers/exercises.cpp
// Fill in each TODO. Run the binary — CHECK() failures tell you what's wrong.
// Don't change function signatures or the test.hpp macros.
#include "test.hpp"
#include <cstddef>

// TODO 1: swap two ints using pointers (no std::swap, no references).
void swap_via_pointers(int* a, int* b) {
    // your code here
}

// TODO 2: return a pointer to the larger of *a and *b (not a copy of the value).
int* max_via_pointers(int* a, int* b) {
    // your code here
    return nullptr;
}

// TODO 3: given a C-style array and its length, return the number of times
// `target` appears, walking the array using pointer arithmetic (p, p+1, ...)
// rather than arr[i] indexing.
int count_via_pointer_walk(int* arr, int len, int target) {
    // your code here
    return -1;
}

// TODO 4: reverse the array in place using two pointers that walk toward
// each other from both ends.
void reverse_in_place(int* arr, int len) {
    // your code here
}

int main() {
    int a = 3, b = 7;
    swap_via_pointers(&a, &b);
    CHECK_EQ(a, 7);
    CHECK_EQ(b, 3);

    int x = 10, y = 20;
    int* m = max_via_pointers(&x, &y);
    CHECK(m == &y);
    CHECK_EQ(*m, 20);

    int arr[] = {1, 2, 3, 2, 2, 5};
    CHECK_EQ(count_via_pointer_walk(arr, 6, 2), 3);
    CHECK_EQ(count_via_pointer_walk(arr, 6, 9), 0);

    int arr2[] = {1, 2, 3, 4, 5};
    reverse_in_place(arr2, 5);
    CHECK_EQ(arr2[0], 5);
    CHECK_EQ(arr2[1], 4);
    CHECK_EQ(arr2[2], 3);
    CHECK_EQ(arr2[3], 2);
    CHECK_EQ(arr2[4], 1);

    int arr3[] = {1, 2, 3, 4};
    reverse_in_place(arr3, 4);
    CHECK_EQ(arr3[0], 4);
    CHECK_EQ(arr3[3], 1);

    TEST_SUMMARY();
}
