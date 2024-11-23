#pragma once

#include <cmath>
#include <limits>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"

consteval double sqrt_constexpr(double val) {
  /// Constexpr version of the square root
  /// Return value:
  ///   - For a finite and non-negative value of "x", returns an approximation for the square root of "x"
  ///   - Otherwise, returns NaN
  // See https://stackoverflow.com/a/79163097/1678468

  // TODO: with full C++23 support, use std::signbit as it should be constexpr
  if(val < 0.0) return std::numeric_limits<double>::quiet_NaN();
  if(val == std::numeric_limits<double>::infinity()) return std::numeric_limits<double>::quiet_NaN();

  double result{val};
  for(double last{0.0}; result != last; result = 0.5 * (result + val / result)) last = result;
  return result;
}

#pragma GCC diagnostic pop
