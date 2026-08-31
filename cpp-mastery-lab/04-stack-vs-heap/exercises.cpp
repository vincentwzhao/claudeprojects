// 04-stack-vs-heap/exercises.cpp
#include "test.hpp"

// TODO 1: this function has a bug — it returns a pointer to a stack local.
// Fix it by heap-allocating instead (caller will delete it) OR by changing
// the signature to return by value. Keep the *signature* returning int*,
// and heap-allocate, to practice recognizing/fixing this exact class of bug.
int* make_answer() {
    int local = 42;
    return &local;   // BUG: dangling — fix this
}

// TODO 2: sum_stack must NOT heap-allocate. Use a fixed-size local array
// (n is always <= 16 in the tests) to sum the first n integers 0..n-1.
int sum_stack(int n) {
    // your code here
    return -1;
}

// TODO 3: sum_heap must heap-allocate an array of size n (n arbitrary,
// possibly large), sum 0..n-1, free it, and return the sum. This is the
// same computation as sum_stack but for sizes only known at runtime.
long sum_heap(int n) {
    // your code here
    return -1;
}

int main() {
    int* p = make_answer();
    CHECK(p != nullptr);
    CHECK_EQ(*p, 42);
    delete p;   // only correct if make_answer heap-allocated

    CHECK_EQ(sum_stack(5), 0 + 1 + 2 + 3 + 4);
    CHECK_EQ(sum_stack(1), 0);

    CHECK_EQ(sum_heap(1000), 999L * 1000L / 2L);
    CHECK_EQ(sum_heap(1), 0L);

    TEST_SUMMARY();
}
