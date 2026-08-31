# 03 — Memory allocation: solutions

```cpp
int* make_range(int n) {
    int* p = new int[n];
    for (int i = 0; i < n; ++i) p[i] = i;
    return p;
}

void free_range(int* p) {
    delete[] p;
}

int* deep_copy(const int* src, int n) {
    int* dst = new int[n];
    for (int i = 0; i < n; ++i) dst[i] = src[i];
    return dst;
}

int* manual_resize(int* p, int old_n, int new_n) {
    int* fresh = static_cast<int*>(malloc(sizeof(int) * new_n));
    int copy_n = old_n < new_n ? old_n : new_n;
    for (int i = 0; i < copy_n; ++i) fresh[i] = p[i];
    free(p);
    return fresh;
}
```

Notes:
- `manual_resize` is exactly the leak-prone shape `realloc` hides from you:
  if you forget to reassign the return value (`p = manual_resize(p, ...)`)
  and the block moved, you've leaked the old block *and* you're holding a
  dangling pointer. This is why real code uses `std::vector`, which handles
  growth internally — see `07-stl`.
- `deep_copy` matters because a shallow copy (just copying the pointer
  value) means two variables think they own the same heap block — the
  seed for a double-free later.
