#pragma once

// Each test_*() runs its checks and returns the number that failed.

int test_matrix();
int test_gemm_correctness();
int test_ops();
int test_thread_pool();
