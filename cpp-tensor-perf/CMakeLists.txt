cmake_minimum_required(VERSION 3.16)
project(cpp_tensor_perf CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

# -mavx2 -mfma: the SIMD kernel (src/gemm_simd.cpp) uses AVX2/FMA
# intrinsics directly, so it must be compiled for a target that has them.
# Not -march=native: that would make the build's target CPU (and whether
# it even compiles) depend on whatever machine happens to run cmake,
# which is the opposite of reproducible for something meant to build in
# CI and on a reviewer's machine.
add_library(tensorperf STATIC
  src/gemm_naive.cpp
  src/gemm_reordered.cpp
  src/gemm_blocked.cpp
  src/gemm_simd.cpp
  src/gemm_threaded.cpp
  src/thread_pool.cpp
  src/ops.cpp
)
target_include_directories(tensorperf PUBLIC include)
target_compile_options(tensorperf PUBLIC -O3 -Wall -Wextra -mavx2 -mfma)
target_link_libraries(tensorperf PUBLIC Threads::Threads)

add_executable(run_tests
  tests/main.cpp
  tests/test_matrix.cpp
  tests/test_gemm_correctness.cpp
  tests/test_ops.cpp
  tests/test_thread_pool.cpp
)
target_link_libraries(run_tests PRIVATE tensorperf)

add_executable(bench_gemm benchmark/bench_gemm.cpp)
target_link_libraries(bench_gemm PRIVATE tensorperf)

add_executable(mlp_demo examples/mlp_inference_demo.cpp)
target_link_libraries(mlp_demo PRIVATE tensorperf)

enable_testing()
add_test(NAME unit_tests COMMAND run_tests)
