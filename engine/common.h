#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Eigen/Dense>

// Project 0 — automotive simulation core
// Namespace: p0
namespace p0 {

// 2D/3D aliases for Eigen types used across the simulation
using Vec2 = Eigen::Vector2d;
using Vec3 = Eigen::Vector3d;
using Vec4 = Eigen::Vector4d;
using Mat2 = Eigen::Matrix2d;
using Mat3 = Eigen::Matrix3d;
using Mat4 = Eigen::Matrix4d;

// Mathematical and physical constants
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;
constexpr double kHalfPi = 0.5 * kPi;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kRadToDeg = 180.0 / kPi;
constexpr double kGravity = 9.80665;        // m/s^2
constexpr double kEpsilon = 1e-9;           // numerical tolerance

// Clamp value to [min_val, max_val]
inline double clamp(double value, double min_val, double max_val) {
  if (value < min_val) return min_val;
  if (value > max_val) return max_val;
  return value;
}

// Wrap angle to [-pi, +pi]
inline double normalize_angle(double angle) {
  if (!std::isfinite(angle)) return 0.0;
  while (angle > kPi) angle -= kTwoPi;
  while (angle < -kPi) angle += kTwoPi;
  return angle;
}

// Linear interpolation for scalars
inline double lerp(double a, double b, double t) {
  return a + (b - a) * t;
}

// Linear interpolation for Vec2
inline Vec2 lerp(const Vec2& a, const Vec2& b, double t) {
  return (1.0 - t) * a + t * b;
}

}
