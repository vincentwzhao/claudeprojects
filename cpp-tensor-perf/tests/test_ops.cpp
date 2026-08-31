#include "tensor/ops.hpp"

#include "check.hpp"

int test_ops() {
  int _tt_failures = 0;
  using tensor::Matrix;

  {
    Matrix a(3, 9), b(3, 9), out(3, 9);  // 9 cols: exercises the SIMD tail
    a.FillRandom(1);
    b.FillRandom(2);
    tensor::ops::Add(a, b, out);
    for (std::size_t r = 0; r < 3; ++r)
      for (std::size_t c = 0; c < 9; ++c)
        TT_CHECK_NEAR(out(r, c), a(r, c) + b(r, c), 1e-6);
  }

  {
    Matrix m(4, 10);
    m.FillRandom(3);
    Matrix bias(1, 10);
    bias.FillRandom(4);

    Matrix before(4, 10);
    for (std::size_t r = 0; r < 4; ++r)
      for (std::size_t c = 0; c < 10; ++c) before(r, c) = m(r, c);

    tensor::ops::AddBiasInPlace(m, bias);
    for (std::size_t r = 0; r < 4; ++r)
      for (std::size_t c = 0; c < 10; ++c)
        TT_CHECK_NEAR(m(r, c), before(r, c) + bias(0, c), 1e-6);
  }

  {
    Matrix m(2, 11);
    m(0, 0) = -5.0f;
    m(0, 1) = 5.0f;
    m(0, 2) = 0.0f;
    tensor::ops::ReluInPlace(m);
    TT_CHECK_NEAR(m(0, 0), 0.0f, 1e-9);
    TT_CHECK_NEAR(m(0, 1), 5.0f, 1e-9);
    TT_CHECK_NEAR(m(0, 2), 0.0f, 1e-9);
  }

  {
    Matrix m(1, 3);
    m(0, 0) = 0.0f;
    m(0, 1) = 100.0f;
    m(0, 2) = -100.0f;
    tensor::ops::SigmoidInPlace(m);
    TT_CHECK_NEAR(m(0, 0), 0.5f, 1e-6);
    TT_CHECK(m(0, 1) > 0.999f);
    TT_CHECK(m(0, 2) < 0.001f);
  }

  return _tt_failures;
}
