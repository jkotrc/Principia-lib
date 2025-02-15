﻿
#pragma once

#include "numerics/fixed_arrays.hpp"

#include <algorithm>
#include <vector>

#include "glog/logging.h"
#include "quantities/elementary_functions.hpp"

namespace principia {
namespace numerics {
namespace internal_fixed_arrays {

using base::uninitialized;
using quantities::Pow;
using quantities::Sqrt;
using quantities::Square;

// A helper class to compute the dot product of two arrays.  |ScalarLeft| and
// |ScalarRight| are the types of the elements of the arrays.  |Left| and
// |Right| are the (deduced) types of the arrays.  They must both have an
// operator[].  |size| is the size of the arrays.
template<typename ScalarLeft, typename ScalarRight, int size, int i = size - 1>
struct DotProduct {
  template<typename Left, typename Right>
  static Product<ScalarLeft, ScalarRight> Compute(Left const& left,
                                                  Right const& right);
};

template<typename ScalarLeft, typename ScalarRight, int size>
struct DotProduct<ScalarLeft, ScalarRight, size, 0> {
  template<typename Left, typename Right>
  static Product<ScalarLeft, ScalarRight> Compute(Left const& left,
                                                  Right const& right);
};

template<typename ScalarLeft, typename ScalarRight, int size, int i>
template<typename Left, typename Right>
Product<ScalarLeft, ScalarRight>
DotProduct<ScalarLeft, ScalarRight, size, i>::Compute(Left const& left,
                                                      Right const& right) {
  return left[i] * right[i] +
         DotProduct<ScalarLeft, ScalarRight, size, i - 1>::Compute(left, right);
}

template<typename ScalarLeft, typename ScalarRight, int size>
template<typename Left, typename Right>
Product<ScalarLeft, ScalarRight>
DotProduct<ScalarLeft, ScalarRight, size, 0>::Compute(Left const& left,
                                                      Right const& right) {
  return left[0] * right[0];
}

// The |data_| member is aggregate-initialized with an empty list initializer,
// which performs value initialization on the components.  For quantities this
// calls the default constructor, for non-class types this does
// zero-initialization.
template<typename Scalar, int size_>
constexpr FixedVector<Scalar, size_>::FixedVector() : data_{} {}

template<typename Scalar, int size_>
FixedVector<Scalar, size_>::FixedVector(uninitialized_t) {}

template<typename Scalar, int size_>
constexpr FixedVector<Scalar, size_>::FixedVector(
    std::array<Scalar, size_> const& data)
    : data_(data) {}

template<typename Scalar, int size_>
constexpr FixedVector<Scalar, size_>::FixedVector(
    std::array<Scalar, size_>&& data)
    : data_(std::move(data)) {}

template<typename Scalar, int size_>
TransposedView<FixedVector<Scalar, size_>>
FixedVector<Scalar, size_>::Transpose() const {
  return {.transpose = *this};
}

template<typename Scalar, int size_>
Scalar FixedVector<Scalar, size_>::Norm() const {
  Square<Scalar> norm²{};
  for (auto const c : data_) {
    norm² += c * c;
  }
  return Sqrt(norm²);
}

template<typename Scalar, int size_>
constexpr Scalar& FixedVector<Scalar, size_>::operator[](int const index) {
  CONSTEXPR_DCHECK(0 <= index);
  CONSTEXPR_DCHECK(index < size());
  return data_[index];
}

template<typename Scalar, int size_>
constexpr Scalar const& FixedVector<Scalar, size_>::operator[](
    int const index) const {
  CONSTEXPR_DCHECK(0 <= index);
  CONSTEXPR_DCHECK(index < size());
  return data_[index];
}

template<typename Scalar, int size_>
FixedVector<Scalar, size_>::operator std::vector<Scalar>() const {
  std::vector<Scalar> result(data_.size());
  std::copy(data_.begin(), data_.end(), result.begin());
  return result;
}

template<typename Scalar, int size_>
bool FixedVector<Scalar, size_>::operator==(FixedVector const& right) const {
  return data_ == right.data_;
}

template<typename Scalar, int size_>
bool FixedVector<Scalar, size_>::operator!=(FixedVector const& right) const {
  return data_ != right.data_;
}

template<typename Scalar, int rows_, int columns_>
constexpr FixedMatrix<Scalar, rows_, columns_>::FixedMatrix()
    : data_{} {}

template<typename Scalar, int rows_, int columns_>
FixedMatrix<Scalar, rows_, columns_>::FixedMatrix(uninitialized_t) {}

template<typename Scalar, int rows_, int columns_>
constexpr FixedMatrix<Scalar, rows_, columns_>::FixedMatrix(
    std::array<Scalar, size()> const& data)
    : data_(data) {}

template<typename Scalar, int rows_, int columns_>
constexpr Scalar& FixedMatrix<Scalar, rows_, columns_>::operator()(
    int const row, int const column) {
  CONSTEXPR_DCHECK(0 <= row);
  CONSTEXPR_DCHECK(row < rows());
  CONSTEXPR_DCHECK(0 <= column);
  CONSTEXPR_DCHECK(column < columns());
  return data_[row * columns() + column];
}

template<typename Scalar, int rows_, int columns_>
constexpr Scalar const& FixedMatrix<Scalar, rows_, columns_>::operator()(
    int const row, int const column) const {
  CONSTEXPR_DCHECK(0 <= row);
  CONSTEXPR_DCHECK(row < rows());
  CONSTEXPR_DCHECK(0 <= column);
  CONSTEXPR_DCHECK(column < columns());
  return data_[row * columns() + column];
}

template<typename Scalar, int rows_, int columns_>
template<int r>
Scalar const* FixedMatrix<Scalar, rows_, columns_>::row() const {
  static_assert(r < rows_);
  return &data_[r * columns()];
}

template<typename Scalar, int rows_, int columns_>
FixedMatrix<Scalar, rows_, columns_>
FixedMatrix<Scalar, rows_, columns_>::Transpose() const {
  FixedMatrix<Scalar, rows(), columns()> m(uninitialized);
  for (int i = 0; i < rows(); ++i) {
    for (int j = 0; j < columns(); ++j) {
      m(j, i) = (*this)(i, j);
    }
  }
  return m;
}

template<typename Scalar, int rows_, int columns_>
Scalar FixedMatrix<Scalar, rows_, columns_>::FrobeniusNorm() const {
  Square<Scalar> Σᵢⱼaᵢⱼ²{};
  for (int i = 0; i < rows(); ++i) {
    for (int j = 0; j < columns(); ++j) {
      Σᵢⱼaᵢⱼ² += Pow<2>((*this)(i, j));
    }
  }
  return Sqrt(Σᵢⱼaᵢⱼ²);
}

template<typename Scalar, int rows_, int columns_>
bool FixedMatrix<Scalar, rows_, columns_>::operator==(
    FixedMatrix const& right) const {
  return data_ == right.data_;
}

template<typename Scalar, int rows_, int columns_>
bool FixedMatrix<Scalar, rows_, columns_>::operator!=(
    FixedMatrix const& right) const {
  return data_ != right.data_;
}

template<typename Scalar, int rows_, int columns_>
FixedMatrix<Scalar, rows_, columns_>
FixedMatrix<Scalar, rows_, columns_>::Identity() {
  FixedMatrix<Scalar, rows(), columns()> m;
  for (int i = 0; i < rows(); ++i) {
    m(i, i) = 1;
  }
  return m;
}

template<typename Scalar, int rows_>
constexpr FixedStrictlyLowerTriangularMatrix<Scalar, rows_>::
    FixedStrictlyLowerTriangularMatrix()
    : data_{} {}

template<typename Scalar, int rows_>
FixedStrictlyLowerTriangularMatrix<Scalar, rows_>::
    FixedStrictlyLowerTriangularMatrix(uninitialized_t) {}

template<typename Scalar, int rows_>
constexpr FixedStrictlyLowerTriangularMatrix<Scalar, rows_>::
FixedStrictlyLowerTriangularMatrix(std::array<Scalar, size()> const& data)
    : data_(data) {}

template<typename Scalar, int rows_>
constexpr Scalar& FixedStrictlyLowerTriangularMatrix<Scalar, rows_>::
operator()(int const row, int const column) {
  CONSTEXPR_DCHECK(0 <= column);
  CONSTEXPR_DCHECK(column < row);
  CONSTEXPR_DCHECK(row < rows());
  return data_[row * (row - 1) / 2 + column];
}

template<typename Scalar, int rows_>
constexpr Scalar const& FixedStrictlyLowerTriangularMatrix<Scalar, rows_>::
operator()(int const row, int const column) const {
  CONSTEXPR_DCHECK(0 <= column);
  CONSTEXPR_DCHECK(column < row);
  CONSTEXPR_DCHECK(row < rows());
  return data_[row * (row - 1) / 2 + column];
}

template<typename Scalar, int rows_>
template<int r>
Scalar const* FixedStrictlyLowerTriangularMatrix<Scalar, rows_>::row() const {
  static_assert(r < rows_);
  return &data_[r * (r - 1) / 2];
}

template<typename Scalar, int rows_>
bool FixedStrictlyLowerTriangularMatrix<Scalar, rows_>::operator==(
    FixedStrictlyLowerTriangularMatrix const& right) const {
  return data_ == right.data_;
}

template<typename Scalar, int rows_>
bool FixedStrictlyLowerTriangularMatrix<Scalar, rows_>::operator!=(
    FixedStrictlyLowerTriangularMatrix const& right) const {
  return data_ != right.data_;
}

template<typename Scalar, int rows_>
constexpr FixedLowerTriangularMatrix<Scalar, rows_>::
FixedLowerTriangularMatrix()
    : data_{} {}

template<typename Scalar, int rows_>
FixedLowerTriangularMatrix<Scalar, rows_>::
FixedLowerTriangularMatrix(uninitialized_t) {}

template<typename Scalar, int rows_>
constexpr FixedLowerTriangularMatrix<Scalar, rows_>::
FixedLowerTriangularMatrix(std::array<Scalar, size()> const& data)
    : data_(data) {}

template<typename Scalar, int rows_>
constexpr Scalar& FixedLowerTriangularMatrix<Scalar, rows_>::
operator()(int const row, int const column) {
  CONSTEXPR_DCHECK(0 <= column);
  CONSTEXPR_DCHECK(column <= row);
  CONSTEXPR_DCHECK(row < rows());
  return data_[row * (row + 1) / 2 + column];
}

template<typename Scalar, int rows_>
constexpr Scalar const& FixedLowerTriangularMatrix<Scalar, rows_>::
operator()(int const row, int const column) const {
  CONSTEXPR_DCHECK(0 <= column);
  CONSTEXPR_DCHECK(column <= row);
  CONSTEXPR_DCHECK(row < rows());
  return data_[row * (row + 1) / 2 + column];
}

template<typename Scalar, int rows_>
bool FixedLowerTriangularMatrix<Scalar, rows_>::operator==(
    FixedLowerTriangularMatrix const& right) const {
  return data_ == right.data_;
}

template<typename Scalar, int rows_>
bool FixedLowerTriangularMatrix<Scalar, rows_>::operator!=(
    FixedLowerTriangularMatrix const& right) const {
  return data_ != right.data_;
}

template<typename Scalar, int columns_>
constexpr FixedUpperTriangularMatrix<Scalar, columns_>::
FixedUpperTriangularMatrix()
    : data_{} {}

template<typename Scalar, int columns_>
FixedUpperTriangularMatrix<Scalar, columns_>::FixedUpperTriangularMatrix(
    uninitialized_t) {}

template<typename Scalar, int columns_>
constexpr FixedUpperTriangularMatrix<Scalar, columns_>::
FixedUpperTriangularMatrix(std::array<Scalar, size()> const& data)
    : data_(Transpose(data)) {}

template<typename Scalar, int columns_>
constexpr Scalar& FixedUpperTriangularMatrix<Scalar, columns_>::
operator()(int const row, int const column) {
  CONSTEXPR_DCHECK(0 <= row);
  CONSTEXPR_DCHECK(row <= column);
  CONSTEXPR_DCHECK(column < columns());
  return data_[column * (column + 1) / 2 + row];
}

template<typename Scalar, int columns_>
constexpr Scalar const& FixedUpperTriangularMatrix<Scalar, columns_>::
operator()(int const row, int const column) const {
  CONSTEXPR_DCHECK(0 <= row);
  CONSTEXPR_DCHECK(row <= column);
  CONSTEXPR_DCHECK(column < columns());
  return data_[column * (column + 1) / 2 + row];
}

template<typename Scalar, int columns_>
bool FixedUpperTriangularMatrix<Scalar, columns_>::operator==(
    FixedUpperTriangularMatrix const& right) const {
  return data_ == right.data_;
}

template<typename Scalar, int columns_>
bool FixedUpperTriangularMatrix<Scalar, columns_>::operator!=(
    FixedUpperTriangularMatrix const& right) const {
  return data_ != right.data_;
}

template<typename Scalar, int columns_>
auto FixedUpperTriangularMatrix<Scalar, columns_>::Transpose(
    std::array<Scalar, size()> const& data)
    -> std::array<Scalar, size()> {
  std::array<Scalar, rows() * columns()> full;
  int index = 0;
  for (int row = 0; row < rows(); ++row) {
    for (int column = row; column < columns(); ++column) {
      full[row * columns() + column] = data[index];
      ++index;
    }
  }

  std::array<Scalar, size()> result;
  index = 0;
  for (int column = 0; column < columns(); ++column) {
    for (int row = 0; row <= column; ++row) {
      result[index] = full[row * columns() + column];
      ++index;
    }
  }
  return result;
}

template<typename ScalarLeft, typename ScalarRight, int size>
constexpr FixedVector<Quotient<ScalarLeft, ScalarRight>, size> operator/(
    FixedVector<ScalarLeft, size> const& left,
    ScalarRight const& right) {
  FixedVector<Quotient<ScalarLeft, ScalarRight>, size> result(uninitialized);
  for (int i = 0; i < left.size(); ++i) {
    result[i] = left[i] / right;
  }
  return result;
}

template<typename ScalarLeft, typename ScalarRight, int size>
constexpr FixedVector<Difference<ScalarLeft, ScalarRight>, size> operator-(
    FixedVector<ScalarLeft, size> const& left,
    FixedVector<ScalarRight, size> const& right) {
  std::array<Difference<ScalarLeft, ScalarRight>, size> result{};
  for (int i = 0; i < size; ++i) {
    result[i] = left[i] - right[i];
  }
  return FixedVector<Difference<ScalarLeft, ScalarRight>, size>(
      std::move(result));
}

template<typename ScalarLeft, typename ScalarRight, int size>
constexpr Product<ScalarLeft, ScalarRight> operator*(
    ScalarLeft* const left,
    FixedVector<ScalarRight, size> const& right) {
  return DotProduct<ScalarLeft, ScalarRight, size>::Compute(left, right.data_);
}

template<typename ScalarLeft, typename ScalarRight, int size>
constexpr Product<ScalarLeft, ScalarRight> operator*(
    TransposedView<FixedVector<ScalarLeft, size>> const& left,
    FixedVector<ScalarRight, size> const& right) {
  return DotProduct<ScalarLeft, ScalarRight, size>::Compute(
      left.transpose.data_, right.data_);
}

template<typename ScalarLeft, typename ScalarRight,
         int rows, int dimension, int columns>
constexpr FixedMatrix<Product<ScalarLeft, ScalarRight>, rows, columns>
operator*(FixedMatrix<ScalarLeft, rows, dimension> const& left,
          FixedMatrix<ScalarRight, dimension, columns> const& right) {
  FixedMatrix<Product<ScalarLeft, ScalarRight>, rows, columns> result;
  for (int i = 0; i < left.rows(); ++i) {
    for (int j = 0; j < right.columns(); ++j) {
      for (int k = 0; k < left.columns(); ++k) {
        result(i, j) += left(i, k) * right(k, j);
      }
    }
  }
  return result;
}

template<typename ScalarLeft, typename ScalarRight, int rows, int columns>
constexpr FixedVector<Product<ScalarLeft, ScalarRight>, rows> operator*(
    FixedMatrix<ScalarLeft, rows, columns> const& left,
    FixedVector<ScalarRight, columns> const& right) {
  std::array<Product<ScalarLeft, ScalarRight>, rows> result;
  auto const* row = left.data_.data();
  for (int i = 0; i < rows; ++i) {
    result[i] =
        DotProduct<ScalarLeft, ScalarRight, columns>::Compute(row, right.data_);
    row += columns;
  }
  return FixedVector<Product<ScalarLeft, ScalarRight>, rows>(std::move(result));
}

template<typename Scalar, int size>
std::ostream& operator<<(std::ostream& out,
                         FixedVector<Scalar, size> const& vector) {
  std::stringstream s;
  for (int i = 0; i < size; ++i) {
    s << (i == 0 ? "{" : "") << vector[i]
      << (i == size - 1 ? "}" : ", ");
  }
  out << s.str();
  return out;
}

template<typename Scalar, int rows, int columns>
std::ostream& operator<<(std::ostream& out,
                         FixedMatrix<Scalar, rows, columns> const& matrix) {
  out << "rows: " << rows << " columns: " << columns << "\n";
  for (int i = 0; i < rows; ++i) {
    out << "{";
    for (int j = 0; j < columns; ++j) {
      out << matrix(i, j);
      if (j < columns - 1) {
        out << ", ";
      }
    }
    out << "}\n";
  }
  return out;
}

template<typename Scalar, int rows>
std::ostream& operator<<(
    std::ostream& out,
    FixedLowerTriangularMatrix<Scalar, rows> const& matrix) {
  out << "rows: " << matrix.rows() << "\n";
  for (int i = 0; i < matrix.rows(); ++i) {
    out << "{";
    for (int j = 0; j <= i; ++j) {
      out << matrix(i, j);
      if (j < i) {
        out << ", ";
      }
    }
    out << "}\n";
  }
  return out;
}

template<typename Scalar, int columns>
std::ostream& operator<<(
    std::ostream& out,
    FixedUpperTriangularMatrix<Scalar, columns> const& matrix) {
  out << "columns: " << matrix.columns() << "\n";
  for (int i = 0; i < matrix.columns(); ++i) {
    out << "{";
    for (int j = i; j < matrix.columns(); ++j) {
      if (j > i) {
        out << ", ";
      }
      out << matrix(i, j);
    }
    out << "}\n";
  }
  return out;
}

}  // namespace internal_fixed_arrays
}  // namespace numerics
}  // namespace principia
