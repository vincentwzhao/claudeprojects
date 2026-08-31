// 07-stl/demo.cpp
#include <cstdio>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <string>

void vector_basics() {
    std::vector<int> v = {5, 3, 1, 4, 2};
    v.push_back(6);
    std::sort(v.begin(), v.end());
    printf("  sorted: ");
    for (int x : v) printf("%d ", x);
    printf("\n");

    auto it = std::find(v.begin(), v.end(), 4);
    printf("  found 4 at index %ld\n", it - v.begin());

    int sum = std::accumulate(v.begin(), v.end(), 0);
    printf("  sum = %d\n", sum);
}

void iterator_invalidation_demo() {
    std::vector<int> v = {1, 2, 3};
    // The SAFE way to grow a copy while scanning the original: iterate over
    // a snapshot of the size, or don't mutate the container you're iterating.
    size_t original_size = v.size();
    for (size_t i = 0; i < original_size; ++i) {
        if (v[i] == 2) v.push_back(99);   // fine: we index by int, and we
    }                                       // don't dereference a stale iterator
    printf("  after conditional push_back: ");
    for (int x : v) printf("%d ", x);
    printf("\n  (note: doing this via a live iterator instead of an index\n");
    printf("   would risk a dangling iterator after reallocation)\n");
}

void map_vs_unordered_map() {
    std::map<std::string, int> ordered = {{"banana", 2}, {"apple", 5}, {"cherry", 1}};
    printf("  std::map iterates in sorted key order:\n");
    for (auto& [k, v] : ordered) printf("    %s -> %d\n", k.c_str(), v);

    std::unordered_map<std::string, int> hashed = {{"banana", 2}, {"apple", 5}};
    // Correct existence check: find(), not operator[] (which would insert).
    if (hashed.find("apple") != hashed.end()) {
        printf("  'apple' found in unordered_map without inserting it\n");
    }
    if (hashed.find("kiwi") == hashed.end()) {
        printf("  'kiwi' correctly reported as absent\n");
    }
}

void algorithms_over_ranges() {
    int raw[] = {9, 1, 5, 3, 7};
    // std::sort works on ANY iterator range — here, raw pointers into a
    // plain array — because algorithms are decoupled from containers.
    std::sort(raw, raw + 5);
    printf("  sorted raw array: ");
    for (int x : raw) printf("%d ", x);
    printf("\n");

    std::vector<int> v = {1, 2, 3, 4, 5};
    bool all_positive = std::all_of(v.begin(), v.end(), [](int x) { return x > 0; });
    int count_even = std::count_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
    printf("  all_positive=%s count_even=%d\n", all_positive ? "true" : "false", count_even);
}

int main() {
    printf("-- vector basics (sort, find, accumulate) --\n");
    vector_basics();
    printf("\n-- iterator invalidation awareness --\n");
    iterator_invalidation_demo();
    printf("\n-- map vs unordered_map --\n");
    map_vs_unordered_map();
    printf("\n-- algorithms decoupled from containers --\n");
    algorithms_over_ranges();
    return 0;
}
