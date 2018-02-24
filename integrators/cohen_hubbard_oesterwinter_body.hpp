﻿
#pragma once

#include "integrators/cohen_hubbard_oesterwinter.hpp"

namespace principia {
namespace integrators {

template<>
inline CohenHubbardOesterwinter<1> const& CohenHubbardOesterwinterOrder<1>() {
  static CohenHubbardOesterwinter<1> const cohen_hubbard_oesterwinter{{1.0},
                                                                      2.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<2> const& CohenHubbardOesterwinterOrder<2>() {
  static CohenHubbardOesterwinter<2> const cohen_hubbard_oesterwinter{
      {2.0, 1.0}, 6.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<3> const& CohenHubbardOesterwinterOrder<3>() {
  static CohenHubbardOesterwinter<3> const cohen_hubbard_oesterwinter{
      {7.0, 6.0, -1.0}, 24.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<4> const& CohenHubbardOesterwinterOrder<4>() {
  static CohenHubbardOesterwinter<4> const cohen_hubbard_oesterwinter{
      {97.0, 114.0, -39.0, 8.0}, 360.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<5> const& CohenHubbardOesterwinterOrder<5>() {
  static CohenHubbardOesterwinter<5> const cohen_hubbard_oesterwinter{
      {367.0, 540.0, -282.0, 116.0, -21.0}, 1440.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<6> const& CohenHubbardOesterwinterOrder<6>() {
  static CohenHubbardOesterwinter<6> const cohen_hubbard_oesterwinter{
      {2462.0, 4315.0, -3044.0, 1882.0, -682.0, 107.0}, 10080.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<7> const& CohenHubbardOesterwinterOrder<7>() {
  static CohenHubbardOesterwinter<7> const cohen_hubbard_oesterwinter{
      {28549.0, 57750.0, -51453.0, 42484.0, -23109.0, 7254.0, -995.0},
      120960.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<8> const& CohenHubbardOesterwinterOrder<8>() {
  static CohenHubbardOesterwinter<8> const cohen_hubbard_oesterwinter{
      {416173.0,
       950684.0,
       -1025097.0,
       1059430.0,
       -768805.0,
       362112.0,
       -99359.0,
       12062.0},
      1814400.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<9> const& CohenHubbardOesterwinterOrder<9>() {
  static CohenHubbardOesterwinter<9> const cohen_hubbard_oesterwinter{
      {1624505.0,
       4124232.0,
       -5225624.0,
       6488192.0,
       -5888310.0,
       3698920.0,
       -1522672.0,
       369744.0,
       -40187.0},
      7257600.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<10> const& CohenHubbardOesterwinterOrder<10>() {
  static CohenHubbardOesterwinter<10> const cohen_hubbard_oesterwinter{
      {52478684.0,
       146269485.0,
       -213124908.0,
       309028740.0,
       -336691836.0,
       264441966.0,
       -145166580.0,
       52880868.0,
       -11496000.0,
       1129981.0},
      239500800.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<11> const& CohenHubbardOesterwinterOrder<11>() {
  static CohenHubbardOesterwinter<11> const cohen_hubbard_oesterwinter{
      {205994615.0,
       624279150.0,
       -1028905077.0,
       1706529480.0,
       -2169992754.0,
       2045638356.0,
       -1403891730.0,
       681937992.0,
       -222389445.0,
       43721134.0,
       -3920121.0},
      958003200.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<12> const& CohenHubbardOesterwinterOrder<12>() {
  static CohenHubbardOesterwinter<12> const cohen_hubbard_oesterwinter{
      {92158447389.0,
       301307140046.0,
       -554452444015.0,
       1035372815340.0,
       -1505150506950.0,
       1655690777412.0,
       -1363696062582.0,
       828085590240.0,
       -360089099415.0,
       106193749950.0,
       -19043781851.0,
       1569102436.0},
      435891456000.0};
  return cohen_hubbard_oesterwinter;
}

template<>
inline CohenHubbardOesterwinter<13> const& CohenHubbardOesterwinterOrder<13>() {
  static CohenHubbardOesterwinter<13> const cohen_hubbard_oesterwinter{
      {1089142980505.0,
       3816786338508.0,
       -7759482946938.0,
       16111319179940.0,
       -26357208224085.0,
       33140932754040.0,
       -31849103413596.0,
       23209670507976.0,
       -12616471333665.0,
       4961170395260.0,
       -1334579000970.0,
       219929887188.0,
       -16758388163.0},
      5230697472000.0};
  return cohen_hubbard_oesterwinter;
}

}  // namespace integrators
}  // namespace principia
