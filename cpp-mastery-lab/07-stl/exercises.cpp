// 07-stl/exercises.cpp
#include "test.hpp"
#include <vector>
#include <map>
#include <algorithm>
#include <string>
#include <unordered_set>

// TODO 1: return a new vector containing only the even numbers from v,
// preserving order. Use std::copy_if (or a manual loop, your call).
std::vector<int> filter_even(const std::vector<int>& v) {
    // your code here
    return {};
}

// TODO 2: given word counts already in a map, return the word with the
// highest count. Assumes the map is non-empty and has a unique max.
std::string most_frequent(const std::map<std::string, int>& counts) {
    // your code here
    return "";
}

// TODO 3: dedupe a vector of ints, preserving first-occurrence order
// (don't just sort+unique — that changes order; use a manual pass, e.g.
// with an unordered_set to track what's been seen).
std::vector<int> dedupe_preserve_order(const std::vector<int>& v) {
    // your code here
    return {};
}

// TODO 4: sort a vector of (name, score) pairs by score descending, using
// std::sort with a custom comparator (lambda).
void sort_by_score_desc(std::vector<std::pair<std::string, int>>& people) {
    // your code here
}

int main() {
    std::vector<int> v = {1, 2, 3, 4, 5, 6};
    auto evens = filter_even(v);
    CHECK_EQ(evens.size(), 3u);
    CHECK_EQ(evens[0], 2);
    CHECK_EQ(evens[1], 4);
    CHECK_EQ(evens[2], 6);

    std::map<std::string, int> counts = {{"a", 3}, {"b", 9}, {"c", 1}};
    CHECK_EQ(most_frequent(counts), std::string("b"));

    std::vector<int> dup = {1, 3, 2, 3, 1, 4};
    auto d = dedupe_preserve_order(dup);
    CHECK_EQ(d.size(), 4u);
    CHECK_EQ(d[0], 1);
    CHECK_EQ(d[1], 3);
    CHECK_EQ(d[2], 2);
    CHECK_EQ(d[3], 4);

    std::vector<std::pair<std::string, int>> people = {{"alice", 70}, {"bob", 95}, {"carl", 82}};
    sort_by_score_desc(people);
    CHECK_EQ(people[0].first, std::string("bob"));
    CHECK_EQ(people[1].first, std::string("carl"));
    CHECK_EQ(people[2].first, std::string("alice"));

    TEST_SUMMARY();
}
