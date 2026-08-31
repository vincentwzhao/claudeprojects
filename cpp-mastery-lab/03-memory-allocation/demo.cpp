// 03-memory-allocation/demo.cpp
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <new>

struct Widget {
    int id;
    Widget(int i) : id(i) { printf("  Widget(%d) constructed\n", id); }
    ~Widget() { printf("  Widget(%d) destroyed\n", id); }
};

void new_delete_single() {
    Widget* w = new Widget(1);      // heap alloc + constructor runs
    printf("  using w->id = %d\n", w->id);
    delete w;                        // destructor runs + heap freed
}

void new_delete_array() {
    Widget* arr = new Widget[3]{Widget(10), Widget(11), Widget(12)};
    delete[] arr;   // MUST use delete[] — delete alone is UB for array new
}

void malloc_free_no_ctor() {
    // malloc gives raw bytes — no constructor runs. Fine for POD, wrong
    // for types with real constructors/destructors (like Widget above).
    int* p = static_cast<int*>(malloc(sizeof(int) * 4));
    if (!p) { printf("  allocation failed\n"); return; }
    for (int i = 0; i < 4; ++i) p[i] = i * i;
    printf("  malloc'd ints: %d %d %d %d\n", p[0], p[1], p[2], p[3]);
    free(p);
}

void alignment_demo() {
    printf("  alignof(char)=%zu alignof(int)=%zu alignof(double)=%zu\n",
           alignof(char), alignof(int), alignof(double));

    struct alignas(16) Aligned16 { int x; };
    Aligned16 a;
    printf("  Aligned16 address %% 16 = %ld (0 means correctly aligned)\n",
           reinterpret_cast<uintptr_t>(&a) % 16);
}

void bad_alloc_check() {
    // Requesting an absurd amount fails predictably rather than crashing
    // silently — new throws std::bad_alloc, which we catch here.
    try {
        void* huge = ::operator new(static_cast<size_t>(-1));
        (void)huge;
    } catch (const std::bad_alloc& e) {
        printf("  caught std::bad_alloc as expected: %s\n", e.what());
    }
}

int main() {
    printf("-- new/delete (single object) --\n");
    new_delete_single();
    printf("\n-- new[]/delete[] (array) --\n");
    new_delete_array();
    printf("\n-- malloc/free (no constructor) --\n");
    malloc_free_no_ctor();
    printf("\n-- alignment --\n");
    alignment_demo();
    printf("\n-- bad_alloc --\n");
    bad_alloc_check();
    return 0;
}
