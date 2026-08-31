#include <cstdio>

#include "tests.hpp"

namespace {
int RunSuite(const char* name, int (*fn)()) {
  std::printf("[ RUN  ] %s\n", name);
  int failures = fn();
  std::printf(failures == 0 ? "[  OK  ] %s\n" : "[ FAIL ] %s (%d failed)\n",
              name, failures);
  return failures;
}
}  // namespace

int main() {
  int total_failures = 0;
  total_failures += RunSuite("test_matrix", &test_matrix);
  total_failures += RunSuite("test_gemm_correctness", &test_gemm_correctness);
  total_failures += RunSuite("test_ops", &test_ops);
  total_failures += RunSuite("test_thread_pool", &test_thread_pool);

  if (total_failures == 0) {
    std::printf("\nAll tests passed.\n");
  } else {
    std::printf("\n%d check(s) failed.\n", total_failures);
  }
  return total_failures == 0 ? 0 : 1;
}
