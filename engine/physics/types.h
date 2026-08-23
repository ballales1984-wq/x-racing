#pragma once

#include "common.h"

namespace p0::physics {

inline double pacejka_tire_model(double sigma, double mu, double b, double c, double e) {
  return mu * std::sin(c * std::atan(b * sigma - e * (b * sigma - std::atan(b * sigma))));
}

inline Vec2 project_onto_vector(const Vec2& v, const Vec2& onto) {
  return onto * (v.dot(onto) / onto.squaredNorm());
}

inline Vec2 reject_from_vector(const Vec2& v, const Vec2& from) {
  return v - project_onto_vector(v, from);
}

inline double cross2(const Vec2& a, const Vec2& b) {
  return a.x() * b.y() - a.y() * b.x();
}

}
