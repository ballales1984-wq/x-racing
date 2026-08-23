#include "track/track.h"
#include "physics/types.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace p0::track {

Track::Track(const TrackParams& params) {
  total_length_ = params.total_length;
  default_width_ = params.default_width;
  default_friction_ = params.default_friction;
  build_default_track();
}

void Track::build_default_track() {
  points_.clear();
  const int num_points = 200;
  const double step = total_length_ / num_points;

  for (int i = 0; i <= num_points; ++i) {
    const double t = static_cast<double>(i) / num_points;
    const double s = t * total_length_;

    Vec2 pos;
    Vec2 tangent;

    if (t < 0.25) {
      double lt = t / 0.25;
      pos = Vec2(lt * 200.0, 0.0);
      tangent = Vec2(1.0, 0.0);
    } else if (t < 0.5) {
      double lt = (t - 0.25) / 0.25;
      const double cx = 200.0;
      const double cy = -150.0;
      const double r = 150.0;
      const double angle = -kHalfPi + lt * kPi;
      pos = Vec2(cx + r * std::cos(angle), cy + r * std::sin(angle));
      tangent = Vec2(-std::sin(angle), std::cos(angle));
    } else if (t < 0.75) {
      double lt = (t - 0.5) / 0.25;
      pos = Vec2(200.0 - lt * 400.0, -300.0);
      tangent = Vec2(-1.0, 0.0);
    } else {
      double lt = (t - 0.75) / 0.25;
      const double cx = -200.0;
      const double cy = -150.0;
      const double r = 150.0;
      const double angle = kPi + lt * kPi;
      pos = Vec2(cx + r * std::cos(angle), cy + r * std::sin(angle));
      tangent = Vec2(-std::sin(angle), std::cos(angle));
    }

    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());

    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.distance = s;
    point.curvature = 0.0;
    point.banking = 0.0;

    points_.push_back(point);
  }

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
