
#pragma once

namespace principia {
namespace quantities {
namespace internal_generators {

// These structs have a |Type| member that is a |Quantity| suitable for
// the result of the operation applied to argument(s) of the |Quantity| types
// given as template parameter(s).

template<typename Q, int n>
struct ExponentiationGenerator;

// Only legal if |n| divides the dimensions of |Q|.
template<typename Q, int n, typename = void>
struct NthRootGenerator;

template<typename Left, typename Right>
struct ProductGenerator;

template<typename Left, typename Right>
struct QuotientGenerator;

}  // namespace internal_generators
}  // namespace quantities
}  // namespace principia

#include "quantities/generators_body.hpp"
