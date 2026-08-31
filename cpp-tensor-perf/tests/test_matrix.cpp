#include "tensor/matrix.hpp"

#include "check.hpp"

int test_matrix() {
  int _tt_failures = 0;
  using tensor::Matrix;

  {
    Matrix m(3, 5);
    TT_CHECK(m.rows() == 3);
    TT_CHECK(m.cols() == 5);
    TT_CHECK(m.stride() >= m.cols());
    // Freshly constructed matrices are zero-filled.
    for (std::size_t r = 0; r < m.rows(); ++r)
      for (std::size_t c = 0; c < m.cols(); ++c)
        TT_CHECK_NEAR(m(r, c), 0.0f, 1e-9);
  }

  {
    Matrix m(2, 2);
    m(0, 0) = 1.0f;
    m(0, 1) = 2.0f;
    m(1, 0) = 3.0f;
    m(1, 1) = 4.0f;
    TT_CHECK_NEAR(m(0, 0), 1.0f, 1e-9);
    TT_CHECK_NEAR(m(1, 1), 4.0f, 1e-9);
  }

  {
    Matrix m(4, 4);
    m.Fill(7.0f);
    for (std::size_t r = 0; r < m.rows(); ++r)
      for (std::size_t c = 0; c < m.cols(); ++c)
        TT_CHECK_NEAR(m(r, c), 7.0f, 1e-9);
  }

  {
    // AllClose should catch a real mismatch and accept tiny FP noise.
    Matrix a(2, 2), b(2, 2);
    a.FillRandom(42);
    for (std::size_t r = 0; r < 2; ++r)
      for (std::size_t c = 0; c < 2; ++c) b(r, c) = a(r, c);
    TT_CHECK(a.AllClose(b));

    b(0, 0) += 1.0f;
    TT_CHECK(!a.AllClose(b));

    b(0, 0) -= 1.0f;
    b(0, 0) += 1e-6f;  // within default tolerance
    TT_CHECK(a.AllClose(b));
  }

  return _tt_failures;
}
