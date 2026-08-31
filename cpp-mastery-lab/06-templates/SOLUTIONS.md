# 06 — Templates: solutions

```cpp
template <typename T>
T min_of(T a, T b) { return a < b ? a : b; }

template <typename First, typename Second>
Pair<First, Second> make_pair_of(First f, Second s) {
    return Pair<First, Second>(f, s);
}

template <typename T>
T sum_all(const T* arr, int len) {
    T total = T{};
    for (int i = 0; i < len; ++i) total = total + arr[i];
    return total;
}

template <typename T>
T clamp_value(T value, T lo, T hi) {
    if (value < lo) return lo;
    if (value > hi) return hi;
    return value;
}
```

Notes:
- `sum_all` and `clamp_value` compile for *any* `T` that supports the
  operators used (`+`, `<`, `>`) — that's the whole value proposition of
  templates: one definition, works for `int`, `double`, or any custom type
  with those operators overloaded.
- This is literally what `std::min`/`std::max`/`std::clamp`/`std::accumulate`
  are, minus the header-file plumbing — `07-stl` picks up from here and
  shows you the real, tested, standard versions instead of reinventing them.
