// 11-debugging/buggy.cpp
//
// Six deliberate memory bugs, one per numbered mode. Run with an argument
// 1-6 to trigger a specific one. See README.md for how to diagnose each
// with ASan/UBSan, valgrind, and gdb. Compare against fixed.cpp once
// you've found each bug yourself.
#include <cstdio>
#include <cstdlib>
#include <cstring>

void bug1_use_after_free() {
    int* p = new int(42);
    delete p;
    printf("use-after-free: *p = %d\n", *p);   // BUG: p's memory was already freed
}

void bug2_double_free() {
    int* p = new int(7);
    delete p;
    delete p;                                    // BUG: freeing the same pointer twice
    printf("double-free: (if we get here, still corrupted the heap)\n");
}

void bug3_heap_buffer_overflow() {
    int* arr = new int[5];
    for (int i = 0; i <= 5; ++i) {   // BUG: off-by-one, i==5 is out of bounds
        arr[i] = i;
    }
    printf("heap overflow: arr[4]=%d (arr[5] write was already out of bounds)\n", arr[4]);
    delete[] arr;
}

void bug4_memory_leak() {
    for (int i = 0; i < 3; ++i) {
        int* leaked = new int(i);    // BUG: never deleted, and the pointer
        (void)leaked;                  // goes out of scope every iteration
    }
    printf("memory leak: leaked 3 ints, nothing frees them\n");
}

void bug5_uninitialized_read() {
    int x;                              // BUG: never initialized
    if (x > 0) {                        // reading garbage — UB
        printf("uninitialized read: x was 'positive' garbage (%d)\n", x);
    } else {
        printf("uninitialized read: x was 'non-positive' garbage (%d)\n", x);
    }
}

void bug6_stack_buffer_overflow() {
    int buf[4];
    for (int i = 0; i <= 4; ++i) {    // BUG: off-by-one, writes buf[4] (out of bounds)
        buf[i] = i * i;
    }
    printf("stack overflow: buf[3]=%d (buf[4] write already corrupted adjacent stack memory)\n", buf[3]);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s <1-6>\n", argv[0]);
        printf("  1 = use-after-free\n  2 = double-free\n  3 = heap buffer overflow\n");
        printf("  4 = memory leak\n  5 = uninitialized read\n  6 = stack buffer overflow\n");
        return 1;
    }
    switch (atoi(argv[1])) {
        case 1: bug1_use_after_free(); break;
        case 2: bug2_double_free(); break;
        case 3: bug3_heap_buffer_overflow(); break;
        case 4: bug4_memory_leak(); break;
        case 5: bug5_uninitialized_read(); break;
        case 6: bug6_stack_buffer_overflow(); break;
        default:
            printf("unknown mode '%s'\n", argv[1]);
            return 1;
    }
    return 0;
}
