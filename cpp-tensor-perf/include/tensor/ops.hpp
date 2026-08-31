#pragma once

#include "tensor/matrix.hpp"

// Elementwise ops used to turn a bare matmul into an ML forward pass
// (matmul -> add bias -> activation). Each is AVX2-vectorized with a
// scalar remainder tail; unlike gemm.hpp there's no naive/optimized
// progression here since the point of this project is GEMM, not these —
// they exist so examples/mlp_inference_demo.cpp reads as a real network
// layer instead of a bare matmul call.
namespace tensor::ops {

// out = a + b, elementwise. All three must have the same shape.
void Add(const Matrix& a, const Matrix& b, Matrix& out);

// Adds bias (a 1 x cols row vector) to every row of m, in place. This is
// the "+ b" in "y = xW + b" for a fully-connected layer.
void AddBiasInPlace(Matrix& m, const Matrix& bias_row);

// max(x, 0), in place.
void ReluInPlace(Matrix& m);

// 1 / (1 + exp(-x)), in place. Not vectorized (exp() has no portable
// intrinsic used here) — fine for a demo's output layer, called on a
// tensor orders of magnitude smaller than the hidden-layer matmuls it
// follows.
void SigmoidInPlace(Matrix& m);

}  // namespace tensor::ops
