// 11-debugging/fixed.cpp
//
// Corrected versions of every bug in buggy.cpp. Running this binary with no
// arguments runs all six and exits 0 — that's what ctest checks. Read each
// fix side by side with its buggy.cpp counterpart.
#include <cstdio>
#include <cstring>

void fix1_use_after_free() {
    int* p = new int(42);
    int value = *p;      // read the value BEFORE freeing
    delete p;
    p = nullptr;          // optional but good practice: prevents accidental reuse
    printf("fix1 (use-after-free): value = %d\n", value);
}

void fix2_double_free() {
    int* p = new int(7);
    delete p;
    p = nullptr;           // deleting nullptr is always a safe no-op
    delete p;               // this is now well-defined (does nothing)
    printf("fix2 (double-free): safe, deleting nullptr twice is a no-op\n");
}

void fix3_heap_buffer_overflow() {
    int* arr = new int[5];
    for (int i = 0; i < 5; ++i) {   // fixed: strictly less than, matches allocation size
        arr[i] = i;
    }
    printf("fix3 (heap overflow): arr[4] = %d, no out-of-bounds write\n", arr[4]);
    delete[] arr;
}

void fix4_memory_leak() {
    for (int i = 0; i < 3; ++i) {
        int* p = new int(i);
        printf("fix4 (leak): allocated and immediately freed %d\n", *p);
        delete p;             // freed every iteration — nothing outlives its need
    }
}

void fix5_uninitialized_read() {
    int x = 0;                 // fixed: explicit initialization
    if (x > 0) {
        printf("fix5 (uninitialized read): x was positive (%d)\n", x);
    } else {
        printf("fix5 (uninitialized read): x was non-positive (%d), as expected\n", x);
    }
}

void fix6_stack_buffer_overflow() {
    int buf[4];
    for (int i = 0; i < 4; ++i) {   // fixed: strictly less than, matches array size
        buf[i] = i * i;
    }
    printf("fix6 (stack overflow): buf[3] = %d, no out-of-bounds write\n", buf[3]);
}

int main() {
    fix1_use_after_free();
    fix2_double_free();
    fix3_heap_buffer_overflow();
    fix4_memory_leak();
    fix5_uninitialized_read();
    fix6_stack_buffer_overflow();
    printf("\nall six fixes ran cleanly\n");
    return 0;
}
