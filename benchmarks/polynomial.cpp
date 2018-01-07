﻿
#include <random>
#include <tuple>

#include "astronomy/frames.hpp"
#include "benchmark/benchmark.h"
#include "geometry/grassmann.hpp"
#include "geometry/named_quantities.hpp"
#include "geometry/r3_element.hpp"
#include "numerics/polynomial.hpp"
#include "quantities/quantities.hpp"
#include "quantities/si.hpp"

namespace principia {

using astronomy::ICRFJ2000Ecliptic;
using geometry::Displacement;
using geometry::Multivector;
using geometry::R3Element;
using quantities::Length;
using quantities::Quantity;
using quantities::SIUnit;
using quantities::Time;

namespace numerics {

namespace {
constexpr int evaluations_per_iteration = 1000;
}  // namespace

template<typename T>
struct ValueGenerator;

template<>
struct ValueGenerator<double> {
  static double Get(std::mt19937_64& random) {
    return static_cast<double>(random());
  }
};

template<typename D>
struct ValueGenerator<Quantity<D>> {
  static Quantity<D> Get(std::mt19937_64& random) {
    return static_cast<double>(random()) * SIUnit<Quantity<D>>();
  }
};

template<typename S>
struct ValueGenerator<R3Element<S>> {
  static R3Element<S> Get(std::mt19937_64& random) {
    return {ValueGenerator<S>::Get(random),
            ValueGenerator<S>::Get(random),
            ValueGenerator<S>::Get(random)};
  }
};

template<typename S, typename F, int r>
struct ValueGenerator<Multivector<S, F, r>> {
  static Multivector<S, F, r> Get(std::mt19937_64& random) {
    return Multivector<S, F, r>(ValueGenerator<R3Element<S>>::Get(random));
  }
};

template<typename Tuple, int k, int size = std::tuple_size_v<Tuple>>
struct RandomTupleGenerator {
  static void Fill(Tuple& t, std::mt19937_64& random) {
    std::get<k>(t) =
        ValueGenerator<std::tuple_element_t<k, Tuple>>::Get(random);
    RandomTupleGenerator<Tuple, k + 1, size>::Fill(t, random);
  }
};

template<typename Tuple, int size>
struct RandomTupleGenerator<Tuple, size, size> {
  static void Fill(Tuple& t, std::mt19937_64& random) {}
};

template<typename Value, typename Argument, int degree>
void EvaluatePolynomialInMonomialBasis(benchmark::State& state) {
  using P = PolynomialInMonomialBasis<Value, Argument, degree>;
  std::mt19937_64 random(42);
  P::Coefficients coefficients;
  RandomTupleGenerator<P::Coefficients, 0>::Fill(coefficients, random);
  P const p(coefficients);

  auto const min = ValueGenerator<Argument>::Get(random);
  auto const max = ValueGenerator<Argument>::Get(random);
  auto argument = min;
  auto const Δargument = (max - min) * 1e-9;
  auto result = Value{};

  while (state.KeepRunning()) {
    for (int i = 0; i < evaluations_per_iteration; ++i) {
      result += p.Evaluate(argument);
      argument += Δargument;
    }
  }

  // This weird call to |SetLabel| has no effect except that it uses |result|
  // and therefore prevents the loop from being optimized away.
  std::stringstream ss;
  ss << result;
  state.SetLabel(ss.str().substr(0, 0));
}

void BM_EvaluatePolynomialInMonomialBasisDouble(benchmark::State& state) {
  int const degree = state.range_x();
  switch (degree) {
    case 4:
      EvaluatePolynomialInMonomialBasis<double, Time, 4>(state);
      break;
    case 8:
      EvaluatePolynomialInMonomialBasis<double, Time, 8>(state);
      break;
    case 12:
      EvaluatePolynomialInMonomialBasis<double, Time, 12>(state);
      break;
    case 16:
      EvaluatePolynomialInMonomialBasis<double, Time, 16>(state);
      break;
    default:
      LOG(FATAL) << "Degree " << degree
                 << " in BM_EvaluatePolynomialInMonomialBasisDouble";
  }
}

void BM_EvaluatePolynomialInMonomialBasisQuantity(benchmark::State& state) {
  int const degree = state.range_x();
  switch (degree) {
    case 4:
      EvaluatePolynomialInMonomialBasis<Length, Time, 4>(state);
      break;
    case 8:
      EvaluatePolynomialInMonomialBasis<Length, Time, 8>(state);
      break;
    case 12:
      EvaluatePolynomialInMonomialBasis<Length, Time, 12>(state);
      break;
    case 16:
      EvaluatePolynomialInMonomialBasis<Length, Time, 16>(state);
      break;
    default:
      LOG(FATAL) << "Degree " << degree
                 << " in BM_EvaluatePolynomialInMonomialBasisQuantity";
  }
}

void BM_EvaluatePolynomialInMonomialBasisR3ElementDouble(
    benchmark::State& state) {
  int const degree = state.range_x();
  switch (degree) {
    case 4:
      EvaluatePolynomialInMonomialBasis<R3Element<double>, Time, 4>(state);
      break;
    case 8:
      EvaluatePolynomialInMonomialBasis<R3Element<double>, Time, 8>(state);
      break;
    case 12:
      EvaluatePolynomialInMonomialBasis<R3Element<double>, Time, 12>(state);
      break;
    case 16:
      EvaluatePolynomialInMonomialBasis<R3Element<double>, Time, 16>(state);
      break;
    default:
      LOG(FATAL) << "Degree " << degree
                 << " in BM_EvaluatePolynomialInMonomialBasisR3ElementDouble";
  }
}

void BM_EvaluatePolynomialInMonomialBasisVectorDouble(benchmark::State& state) {
  int const degree = state.range_x();
  switch (degree) {
    case 4:
      EvaluatePolynomialInMonomialBasis<
          Multivector<double, ICRFJ2000Ecliptic, 1>,
          Time,
          4>(state);
      break;
    case 8:
      EvaluatePolynomialInMonomialBasis<
          Multivector<double, ICRFJ2000Ecliptic, 1>,
          Time,
          8>(state);
      break;
    case 12:
      EvaluatePolynomialInMonomialBasis<
          Multivector<double, ICRFJ2000Ecliptic, 1>,
          Time,
          12>(state);
      break;
    case 16:
      EvaluatePolynomialInMonomialBasis<
          Multivector<double, ICRFJ2000Ecliptic, 1>,
          Time,
          16>(state);
      break;
    default:
      LOG(FATAL) << "Degree " << degree
                 << " in BM_EvaluatePolynomialInMonomialBasisVectorDouble";
  }
}

void BM_EvaluatePolynomialInMonomialBasisDisplacement(benchmark::State& state) {
  int const degree = state.range_x();
  switch (degree) {
    case 4:
      EvaluatePolynomialInMonomialBasis<Displacement<ICRFJ2000Ecliptic>,
                                        Time,
                                        4>(state);
      break;
    case 8:
      EvaluatePolynomialInMonomialBasis<Displacement<ICRFJ2000Ecliptic>,
                                        Time,
                                        8>(state);
      break;
    case 12:
      EvaluatePolynomialInMonomialBasis<Displacement<ICRFJ2000Ecliptic>,
                                        Time,
                                        12>(state);
      break;
    case 16:
      EvaluatePolynomialInMonomialBasis<Displacement<ICRFJ2000Ecliptic>,
                                        Time,
                                        16>(state);
      break;
    default:
      LOG(FATAL) << "Degree " << degree
                 << " in BM_EvaluatePolynomialInMonomialBasisDisplacement";
  }
}

BENCHMARK(BM_EvaluatePolynomialInMonomialBasisDouble)
    ->Arg(4)->Arg(8)->Arg(12)->Arg(16);
BENCHMARK(BM_EvaluatePolynomialInMonomialBasisQuantity)
    ->Arg(4)->Arg(8)->Arg(12)->Arg(16);
BENCHMARK(BM_EvaluatePolynomialInMonomialBasisR3ElementDouble)
    ->Arg(4)->Arg(8)->Arg(12)->Arg(16);
BENCHMARK(BM_EvaluatePolynomialInMonomialBasisVectorDouble)
    ->Arg(4)->Arg(8)->Arg(12)->Arg(16);
BENCHMARK(BM_EvaluatePolynomialInMonomialBasisDisplacement)
    ->Arg(4)->Arg(8)->Arg(12)->Arg(16);

}  // namespace numerics
}  // namespace principia
