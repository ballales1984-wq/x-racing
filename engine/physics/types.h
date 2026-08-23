#pragma once

#include "common.h"

// Project 0 — physics primitives and tire model
// Namespace: p0::physics
namespace p0::physics {

// Pacejka Magic Formula tire model (simplified, single coefficient)
// sigma: slip ratio or slip angle
// mu: peak friction coefficient
// b, c, e: shape parameters
inline double pacejka_tire_model(double sigma, double mu, double b, double c, double e) {
  return mu * std::sin(c * std::atan(b * sigma - e * (b * sigma - std::atan(b * sigma))));
}

// Project vector v onto onto (parallel component)
inline Vec2 project_onto_vector(const Vec2& v, const Vec2& onto) {
  return onto * (v.dot(onto) / onto.squaredNorm());
}

// Reject vector v from onto (perpendicular component)
inline Vec2 reject_from_vector(const Vec2& v, const Vec2& from) {
  return v - project_onto_vector(v, from);
}

// 2D cross product (scalar z-component)
inline double cross2(const Vec2& a, const Vec2& b) {
  return a.x() * b.y() - a.y() * b.x();
}

}
