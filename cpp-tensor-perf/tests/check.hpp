#pragma once

#include <cmath>
#include <cstdio>

// Minimal assert-that-doesn't-abort test macros. A failing CHECK prints
// file:line and the failing expression's source text, records the
// failure, and keeps running so one test run reports every failure
// instead of stopping at the first. Each test_*() function returns its
// local failure count; tests/main.cpp sums them and sets the process exit
// code, which is all `ctest` needs.
#define TT_CHECK(cond)                                                    \
  do {                                                                    \
    if (!(cond)) {                                                        \
      std::fprintf(stderr, "  FAIL %s:%d: CHECK(%s)\n", __FILE__,         \
                    __LINE__, #cond);                                     \
      ++_tt_failures;                                                     \
    }                                                                      \
  } while (0)

#define TT_CHECK_NEAR(a, b, tol)                                          \
  do {                                                                    \
    double _tt_a = (a), _tt_b = (b), _tt_tol = (tol);                     \
    if (std::fabs(_tt_a - _tt_b) > _tt_tol) {                             \
      std::fprintf(stderr,                                                \
                    "  FAIL %s:%d: CHECK_NEAR(%s, %s) : %g vs %g "        \
                    "(tol %g)\n",                                         \
                    __FILE__, __LINE__, #a, #b, _tt_a, _tt_b, _tt_tol);   \
      ++_tt_failures;                                                     \
    }                                                                      \
  } while (0)
