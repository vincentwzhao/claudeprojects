# 07 — STL: solutions

```cpp
std::vector<int> filter_even(const std::vector<int>& v) {
    std::vector<int> out;
    std::copy_if(v.begin(), v.end(), std::back_inserter(out),
                 [](int x) { return x % 2 == 0; });
    return out;
}

std::string most_frequent(const std::map<std::string, int>& counts) {
    return std::max_element(counts.begin(), counts.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; })->first;
}

std::vector<int> dedupe_preserve_order(const std::vector<int>& v) {
    std::unordered_set<int> seen;
    std::vector<int> out;
    for (int x : v) {
        if (seen.insert(x).second) out.push_back(x);
    }
    return out;
}

void sort_by_score_desc(std::vector<std::pair<std::string, int>>& people) {
    std::sort(people.begin(), people.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
}
```

Notes:
- `seen.insert(x).second` is `true` exactly when `x` was newly inserted
  (not already present) — a common idiom for "have I seen this before"
  that avoids a separate `find()` + `insert()`.
- `sort_by_score_desc` mutates `people` in place via `std::sort` on the
  full range with a comparator returning `true` when the first argument
  should come *before* the second — `a.second > b.second` gives descending
  order.
