# 02 — References: solutions

```cpp
void swap_via_refs(int& a, int& b) {
    int tmp = a;
    a = b;
    b = tmp;
}

int& max_via_ref(int& a, int& b) {
    return (a >= b) ? a : b;
}

void mark_processed(std::string& s) {
    s += " (processed)";
}

void double_all(std::vector<int>& v) {
    for (int& x : v) x *= 2;   // range-for with a reference binding — no copies
}
```

Notes:
- `double_all` using `for (int x : v)` (by value) would compile fine but do
  nothing to `v` — each `x` is a copy. The `&` in `for (int& x : v)` is the
  entire difference between "read-only view" and "in-place mutation" in a
  range-for loop. This is one of the most common silent-no-op bugs in C++.
- `max_via_ref` returning `int&` (not `int`) is what makes `m = 100;` mutate
  the original `y`. If the signature were `int max_via_ref(...)`, `m` would
  be a copy and mutating it would do nothing to `y`.
