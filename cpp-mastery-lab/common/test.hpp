// Tiny header-only test harness used by every exercises.cpp in this repo.
// No external dependencies (no Catch2/GTest download needed) — just CHECK() and a summary.
#pragma once

#include <iostream>

inline int g_tests_run = 0;
inline int g_tests_failed = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        ++g_tests_run;                                                      \
        if (!(cond)) {                                                      \
            ++g_tests_failed;                                               \
            std::cerr << "  [FAIL] " << __FILE__ << ":" << __LINE__          \
                      << "  CHECK(" #cond ")\n";                            \
        } else {                                                            \
            std::cout << "  [PASS] " #cond "\n";                            \
        }                                                                    \
    } while (0)

#define CHECK_EQ(a, b)                                                       \
    do {                                                                     \
        ++g_tests_run;                                                      \
        auto _a = (a);                                                      \
        auto _b = (b);                                                      \
        if (!(_a == _b)) {                                                  \
            ++g_tests_failed;                                               \
            std::cerr << "  [FAIL] " << __FILE__ << ":" << __LINE__          \
                      << "  CHECK_EQ(" #a ", " #b ") -> " << _a << " != "    \
                      << _b << "\n";                                        \
        } else {                                                            \
            std::cout << "  [PASS] " #a " == " #b "\n";                     \
        }                                                                    \
    } while (0)

#define TEST_SUMMARY()                                                       \
    do {                                                                     \
        std::cout << "\n" << (g_tests_run - g_tests_failed) << "/"           \
                  << g_tests_run << " checks passed\n";                     \
        if (g_tests_failed > 0) {                                           \
            std::cout << "FAILED — fix the TODOs above and re-run.\n";       \
            return 1;                                                       \
        }                                                                    \
        std::cout << "ALL PASSED\n";                                        \
        return 0;                                                           \
    } while (0)
