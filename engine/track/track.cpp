#include "track/track.h"
#include "physics/types.h"
#include <algorithm>
#include <cmath>
#include <numeric>

// Project 0 — parametric track implementation
namespace p0::track {

Track::Track(const TrackParams& params) {
  total_length_ = params.total_length;
  default_width_ = params.default_width;
  default_friction_ = params.default_friction;
  build_default_track();
}

// Build a default closed-loop track composed of:
//   - straight section (0..25%)
//   - right-hand corner (25..50%)
//   - straight section (50..75%)
//   - left-hand corner (75..100%)
void Track::build_default_track() {
  points_.clear();
  const double straightLength = 200.0;
  const double curveRadius = 75.0;
  const int segmentsPerStraight = 100;
  const int segmentsPerCurve = 50;

  for (int i = 0; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    Vec2 pos(t * straightLength, 0.0);
    Vec2 tangent(1.0, 0.0);
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.curvature = 0.0;
    point.banking = 0.0;
    points_.push_back(point);
  }

  for (int i = 1; i <= segmentsPerCurve; ++i) {
    double t = static_cast<double>(i) / segmentsPerCurve;
    double angle = -kHalfPi + t * kPi;
    Vec2 pos(200.0 + curveRadius * std::cos(angle), 75.0 + curveRadius * std::sin(angle));
    Vec2 tangent(-std::sin(angle), std::cos(angle));
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.curvature = 0.0;
    point.banking = 0.0;
    points_.push_back(point);
  }

  for (int i = 1; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    Vec2 pos(200.0 - t * straightLength, 150.0);
    Vec2 tangent(-1.0, 0.0);
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.curvature = 0.0;
    point.banking = 0.0;
    points_.push_back(point);
  }

  for (int i = 1; i <= segmentsPerCurve; ++i) {
    double t = static_cast<double>(i) / segmentsPerCurve;
    double angle = kHalfPi + t * kPi;
    Vec2 pos(curveRadius * std::cos(angle), 75.0 + curveRadius * std::sin(angle));
    Vec2 tangent(-std::sin(angle), std::cos(angle));
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.curvature = 0.0;
    point.banking = 0.0;
    points_.push_back(point);
  }

  for (size_t i = 0; i < points_.size(); ++i) {
    if (i == 0) {
      points_[i].distance = 0.0;
    } else {
      double segmentLength = (points_[i].position - points_[i - 1].position).norm();
      points_[i].distance = points_[i - 1].distance + segmentLength;
    }
  }
  total_length_ = points_.back().distance;

  for (size_t i = 0; i < points_.size(); ++i) {
    const auto& prev = points_[(i > 0) ? i - 1 : points_.size() - 2];
    const auto& next = points_[(i + 1 < points_.size()) ? i + 1 : 1];
    const Vec2 d1 = points_[i].position - prev.position;
    const Vec2 d2 = next.position - points_[i].position;
    const double cross = p0::physics::cross2(d1.normalized(), d2.normalized());
    const double len1 = d1.norm();
    const double len2 = d2.norm();
    if (len1 > kEpsilon && len2 > kEpsilon) {
      points_[i].curvature = cross / ((len1 + len2) * 0.5);
    }
  }
}

// Query track data at distance d (wraps around the loop).
// Returns linearly interpolated geometry between stored points.
TrackPoint Track::at(double distance) const {
  if (points_.empty()) {
    return TrackPoint{};
  }

  distance = std::fmod(distance, total_length_);
  if (distance < 0.0) distance += total_length_;

  const double step = total_length_ / (points_.size() - 1);
  const double raw_index = distance / step;
  const int index = static_cast<int>(std::floor(raw_index));

  if (index >= static_cast<int>(points_.size()) - 1) {
    return points_.back();
  }

  const double frac = raw_index - index;
  return interpolate(distance);
}

// Linear interpolation between two adjacent track points
TrackPoint Track::interpolate(double distance) const {
  const double step = total_length_ / (points_.size() - 1);
  const double raw_index = distance / step;
  const int index = static_cast<int>(std::floor(raw_index));
  const double frac = raw_index - index;

  const int i0 = std::clamp(index, 0, static_cast<int>(points_.size()) - 1);
  const int i1 = std::clamp(index + 1, 0, static_cast<int>(points_.size()) - 1);

  TrackPoint result;
  result.position = lerp(points_[i0].position, points_[i1].position, frac);
  result.tangent = lerp(points_[i0].tangent, points_[i1].tangent, frac).normalized();
  result.normal = Vec2(-result.tangent.y(), result.tangent.x());
  result.curvature = lerp(points_[i0].curvature, points_[i1].curvature, frac);
  result.width = lerp(points_[i0].width, points_[i1].width, frac);
  result.banking = lerp(points_[i0].banking, points_[i1].banking, frac);
  result.friction = lerp(points_[i0].friction, points_[i1].friction, frac);
  result.distance = distance;

  return result;
}

Vec2 Track::get_start_position() const {
  if (points_.empty()) return Vec2::Zero();
  return points_[0].position;
}

double Track::get_start_heading() const {
  if (points_.empty()) return 0.0;
  return std::atan2(points_[0].tangent.y(), points_[0].tangent.x());
}

}
