// 01-pointers/demo.cpp
// Run this, then read it top to bottom next to the output.
#include <cstdio>

void basics() {
    int x = 42;
    int* p = &x;      // p holds the address of x
    printf("x = %d, &x = %p, p = %p, *p = %d\n", x, (void*)&x, (void*)p, *p);

    *p = 100;          // writes through the pointer
    printf("after *p = 100, x = %d\n", x);   // x changed — same memory
}

void pointer_arithmetic() {
    int arr[5] = {10, 20, 30, 40, 50};
    int* p = arr;       // array decays to pointer to its first element

    for (int i = 0; i < 5; ++i) {
        // p + i moves by i * sizeof(int) bytes, not i bytes.
        printf("arr[%d] = %d, *(p+%d) = %d, &arr[%d] - p = %ld\n",
               i, arr[i], i, *(p + i), i, (long)(&arr[i] - p));
    }
}

void const_variants() {
    int a = 1, b = 2;

    const int* p1 = &a;   // pointer to const int: can't do *p1 = ...
    p1 = &b;               // but CAN repoint p1

    int* const p2 = &a;    // const pointer to int: CAN do *p2 = ...
    *p2 = 99;               // but can't repoint p2

    const int* const p3 = &a;  // neither

    printf("a=%d b=%d *p1=%d *p2=%d *p3=%d\n", a, b, *p1, *p2, *p3);
}

// Returns a dangling pointer — DO NOT do this in real code.
// Shown here only so you can see the failure mode.
int* dangling_example() {
    int local = 7;
    return &local;    // local's stack storage is gone once this returns
}

void the_dangling_trap() {
    int* p = dangling_example();
    // Reading *p here is undefined behavior. It might print 7 by luck,
    // print garbage, or crash — that unpredictability IS the bug.
    printf("dangling read (undefined behavior, may look fine or may not): %d\n", *p);
}

void null_and_void_pointers() {
    int x = 5;
    void* vp = &x;          // typeless — can't dereference directly
    int* ip = static_cast<int*>(vp);   // must cast back to use it
    printf("via void*: %d\n", *ip);

    int* np = nullptr;
    printf("nullptr check before use: %s\n", np == nullptr ? "null, skipping deref" : "not null");
}

int add(int a, int b) { return a + b; }
int mul(int a, int b) { return a * b; }

void function_pointers() {
    int (*op)(int, int) = add;   // op points at a function
    printf("op(3,4) via add = %d\n", op(3, 4));
    op = mul;
    printf("op(3,4) via mul = %d\n", op(3, 4));
}

int main() {
    // Line-buffer stdout: the last demo below can crash (real UB), and
    // fully-buffered output would otherwise vanish with it when piped.
    setvbuf(stdout, nullptr, _IOLBF, 0);

    printf("-- basics --\n");
    basics();
    printf("\n-- pointer arithmetic --\n");
    pointer_arithmetic();
    printf("\n-- const variants --\n");
    const_variants();
    printf("\n-- null and void pointers --\n");
    null_and_void_pointers();
    printf("\n-- function pointers --\n");
    function_pointers();
    printf("\n-- the dangling pointer trap --\n");
    the_dangling_trap();
    return 0;
}
