#pragma once

#include "common.h"

// Project 0 — physics primitives and tire model
// Namespace: p0::physics
namespace p0::physics {

// Pacejka Magic Formula tire model (simplified).
// Returns the normalized force shape in [-1, +1]. Callers multiply by D = mu * Fz
// to obtain the physical force. The `mu` parameter was removed because it was
// double-applied: callers in tire_model.h already multiply by (mu * Fz).
// sigma: slip ratio or slip angle
// b, c, e: shape parameters (stiffness, shape, curvature)
inline double pacejka_tire_model(double sigma, double b, double c, double e) {
  return std::sin(c * std::atan(b * sigma - e * (b * sigma - std::atan(b * sigma))));
}

// Project vector v onto onto (parallel component)
inline Vec2 project_onto_vector(const Vec2& v, const Vec2& onto) {
  const double denom = onto.squaredNorm();
  if (denom < kEpsilon) return Vec2::Zero();
  return onto * (v.dot(onto) / denom);
}

// Reject vector v from onto (perpendicular component)
inline Vec2 reject_from_vector(const Vec2& v, const Vec2& from) {
  return v - project_onto_vector(v, from);
}

// 2D cross product (scalar z-component)
inline double cross2(const Vec2& a, const Vec2& b) {
  return a.x() * b.y() - a.y() * b.x();
}

// Centripetal force directed toward the center of curvature.
// Track normal points leftward (90° CCW from tangent). For a left turn
// (positive curvature), the center of curvature is to the left, so the force
// is in the +normal direction: F = m * v^2 * kappa * n.
inline Vec2 centripetal_force(double mass, double speed, double curvature, const Vec2& normal) {
  if (speed < kEpsilon) return Vec2::Zero();
  return mass * speed * speed * curvature * normal;
}

// Centrifugal force (inertial reaction, opposite to centripetal).
inline Vec2 centrifugal_force(double mass, double speed, double curvature, const Vec2& normal) {
  if (speed < kEpsilon) return Vec2::Zero();
  return -mass * speed * speed * curvature * normal;
}

}
