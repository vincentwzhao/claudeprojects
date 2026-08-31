// 04-stack-vs-heap/demo.cpp
#include <cstdio>
#include <chrono>
#include <vector>

void show_stack_growth(int depth) {
    int local = depth;                 // a new stack slot each call
    if (depth <= 3) {
        printf("  depth=%d, &local=%p\n", depth, (void*)&local);
    }
    if (depth < 6) show_stack_growth(depth + 1);
}

void time_stack_vs_heap() {
    constexpr int N = 200000;

    auto t0 = std::chrono::steady_clock::now();
    volatile long sink = 0;
    for (int i = 0; i < N; ++i) {
        int arr[8];               // stack allocation: just moves the stack pointer
        arr[0] = i;
        sink += arr[0];
    }
    auto t1 = std::chrono::steady_clock::now();

    for (int i = 0; i < N; ++i) {
        int* arr = new int[8];    // heap allocation: real allocator work
        arr[0] = i;
        sink += arr[0];
        delete[] arr;
    }
    auto t2 = std::chrono::steady_clock::now();

    auto stack_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    auto heap_us  = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    printf("  %d stack allocs: %lld us | %d heap allocs: %lld us (heap is typically much slower)\n",
           N, (long long)stack_us, N, (long long)heap_us);
}

// The object itself (the vector's control block: pointer, size, capacity)
// lives wherever `v` is declared. The *elements* always live on the heap
// once capacity > 0, regardless of where `v` itself lives.
void object_vs_managed_data() {
    std::vector<int> v = {1, 2, 3};    // v itself: stack. v's buffer: heap.
    printf("  &v (the vector object) = %p  (stack address)\n", (void*)&v);
    printf("  v.data() (the element buffer) = %p  (heap address)\n", (void*)v.data());
}

int main() {
    printf("-- stack grows downward across recursive calls --\n");
    show_stack_growth(0);

    printf("\n-- timing: stack alloc vs heap alloc --\n");
    time_stack_vs_heap();

    printf("\n-- an object's own storage vs. the data it manages --\n");
    object_vs_managed_data();

    return 0;
}
