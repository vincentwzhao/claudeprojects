// 08-raii/exercises.cpp
#include "test.hpp"
#include <memory>
#include <vector>

// TODO 1: implement this to be exception-safe using RAII (a smart pointer).
// The function must:
//  - heap-allocate an int initialized to 0 via std::make_unique<int>
//  - if `should_throw` is true, throw std::runtime_error BEFORE returning
//  - otherwise return the pointed-to value (0)
// With a raw `new`/`delete` instead of a smart pointer, the throwing path
// would leak. Using make_unique, the unique_ptr's destructor runs during
// stack unwinding no matter which path is taken — no leak either way.
// (There's no way to "check" a leak in this test harness, but ASan would
// catch it — see 11-debugging. Do it right anyway.)
int compute(bool should_throw) {
    // your code here
    return -1;
}

// TODO 2: given a vector of raw pointers that all need deleting, convert it
// into a vector of unique_ptr instead so cleanup is automatic. Return the
// vector of unique_ptrs (moves the raw pointers' ownership in).
std::vector<std::unique_ptr<int>> take_ownership(std::vector<int*> raw) {
    // your code here
    return {};
}

// TODO 3: implement a reference-counted "handle" check: given two
// shared_ptr<int> that alias the same object, return true iff they share
// ownership (use_count reflects both) and their use_count equals expected.
bool shares_ownership(const std::shared_ptr<int>& a, const std::shared_ptr<int>& b, long expected_count) {
    // your code here
    return false;
}

int main() {
    CHECK_EQ(compute(false), 0);

    bool threw = false;
    try {
        compute(true);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);

    std::vector<int*> raws = {new int(1), new int(2), new int(3)};
    auto owned = take_ownership(raws);
    CHECK_EQ(owned.size(), 3u);
    CHECK_EQ(*owned[0], 1);
    CHECK_EQ(*owned[2], 3);
    // owned's destructors clean up automatically at end of scope — no delete needed.

    auto sp1 = std::make_shared<int>(42);
    auto sp2 = sp1;
    CHECK(shares_ownership(sp1, sp2, 2));

    TEST_SUMMARY();
}
