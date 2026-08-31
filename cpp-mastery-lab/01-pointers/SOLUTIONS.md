# 01 — Pointers: solutions

```cpp
void swap_via_pointers(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int* max_via_pointers(int* a, int* b) {
    return (*a >= *b) ? a : b;
}

int count_via_pointer_walk(int* arr, int len, int target) {
    int count = 0;
    for (int* p = arr; p != arr + len; ++p) {
        if (*p == target) ++count;
    }
    return count;
}

void reverse_in_place(int* arr, int len) {
    int* lo = arr;
    int* hi = arr + len - 1;
    while (lo < hi) {
        int tmp = *lo;
        *lo = *hi;
        *hi = tmp;
        ++lo;
        --hi;
    }
}
```

Notes:
- `max_via_pointers` returns a *pointer into the caller's memory*, so writing
  through it (`*m = 0;`) would actually mutate `y`. That's the whole point of
  pointers over values — you get aliasing, for better and worse.
- `count_via_pointer_walk` uses `p != arr + len` rather than `p < arr + len`;
  both work for a contiguous array, but `!=` is the idiom STL iterators use
  because it generalizes to non-random-access iterators later.
