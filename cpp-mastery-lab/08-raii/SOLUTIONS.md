# 08 — RAII: solutions

```cpp
int compute(bool should_throw) {
    auto p = std::make_unique<int>(0);
    if (should_throw) throw std::runtime_error("compute failed");
    return *p;
}   // p's destructor runs whether we return normally or throw — no leak either way

std::vector<std::unique_ptr<int>> take_ownership(std::vector<int*> raw) {
    std::vector<std::unique_ptr<int>> out;
    out.reserve(raw.size());
    for (int* p : raw) {
        out.emplace_back(p);   // unique_ptr now owns p; no manual delete needed
    }
    return out;
}

bool shares_ownership(const std::shared_ptr<int>& a, const std::shared_ptr<int>& b, long expected_count) {
    return a.get() == b.get() && a.use_count() == expected_count;
}
```

Notes:
- `compute` is the whole lesson in one function: the *same* smart-pointer
  line handles both the happy path and the exception path correctly,
  because the cleanup is tied to scope exit, not to a specific return
  statement you have to remember to write before every `return`/`throw`.
- `take_ownership` moves ownership of raw pointers you already have (e.g.
  from a C API) into RAII wrappers — a common "adopt legacy pointers"
  pattern. `emplace_back(p)` constructs the `unique_ptr<int>` directly from
  the raw pointer in place.
