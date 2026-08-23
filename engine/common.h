#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <Eigen/Dense>

namespace p0 {

using Vec2 = Eigen::Vector2d;
using Vec3 = Eigen::Vector3d;
using Mat2 = Eigen::Matrix2d;
using Mat3 = Eigen::Matrix3d;

constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;
constexpr double kHalfPi = 0.5 * kPi;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kRadToDeg = 180.0 / kPi;
constexpr double kGravity = 9.80665;
constexpr double kEpsilon = 1e-9;

inline double clamp(double value, double min_val, double max_val) {
  if (value < min_val) return min_val;
  if (value > max_val) return max_val;
  return value;
}

inline double normalize_angle(double angle) {
  while (angle > kPi) angle -= kTwoPi;
  while (angle < -kPi) angle += kTwoPi;
  return angle;
}

inline double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

inline Vec2 lerp(const Vec2& a, const Vec2& b, double t) {
  return (1.0 - t) * a + t * b;
}

}
