#include "track/track.h"
#include "physics/types.h"
#include <algorithm>
#include <cmath>
#include <numeric>

// Project 0 — parametric track implementation
namespace p0::track {

Track::Track(const TrackParams& params)
    : Track(TrackType::Default, params) {}

Track::Track(TrackType type, const TrackParams& params) {
  type_ = type;
  total_length_ = params.total_length;
  default_width_ = params.default_width;
  default_friction_ = params.default_friction;
  default_surface_ = params.default_surface;
  if (type == TrackType::PitCircuit) {
    build_pit_track();
  } else {
    build_default_track();
  }
}

// Build a pit-circuit track composed of:
//   - main straight (0 .. 600 m)
//   - right-hand corner (600 .. ~914 m)
//   - pit-lane straight with pit boxes (~914 .. ~1514 m)
//   - left-hand hairpin closing the loop (~1514 .. ~1828 m)
void Track::build_pit_track() {
  points_.clear();
  pit_box_positions_.clear();
  const double mainStraight = 600.0;
  const double curveRadius = 100.0;
  const double pitStraight = 600.0;
  const int segmentsPerStraight = 120;
  const int segmentsPerCurve = 60;

  // Section 1: main straight heading east (0 .. 600 m).
  for (int i = 0; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    Vec2 pos(t * mainStraight, 0.0);
    Vec2 tangent(1.0, 0.0);
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.surface_type = SurfaceType::Asphalt;
    point.curvature = 0.0;
    point.banking = 0.0;
    point.has_box_lane = false;
    points_.push_back(point);
  }

  // Section 2: right-hand (clockwise) corner (~600 .. ~914 m).
  for (int i = 1; i <= segmentsPerCurve; ++i) {
    double t = static_cast<double>(i) / segmentsPerCurve;
    double angle = kHalfPi - t * kPi;
    Vec2 pos(mainStraight + curveRadius * std::cos(angle),
             -curveRadius + curveRadius * std::sin(angle));
    Vec2 tangent(-std::sin(angle), std::cos(angle));
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.surface_type = SurfaceType::OldAsphalt;
    point.curvature = 0.0;
    point.banking = 0.0;
    point.has_box_lane = false;
    points_.push_back(point);
  }

  // Section 3: pit-lane straight heading west (~914 .. ~1514 m).
  for (int i = 1; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    Vec2 pos(mainStraight - t * pitStraight, -2.0 * curveRadius);
    Vec2 tangent(-1.0, 0.0);
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_ * 0.95;
    point.surface_type = SurfaceType::Asphalt;
    point.curvature = 0.0;
    point.banking = 0.0;
    point.has_box_lane = true;
    point.box_lane_width = 4.0;
    points_.push_back(point);
  }

  // Section 4: left-hand (counter-clockwise) hairpin closing the loop.
  for (int i = 1; i <= segmentsPerCurve; ++i) {
    double t = static_cast<double>(i) / segmentsPerCurve;
    double angle = -kHalfPi + t * kPi;
    Vec2 pos(curveRadius * std::cos(angle),
             -curveRadius + curveRadius * std::sin(angle));
    Vec2 tangent(-std::sin(angle), std::cos(angle));
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.surface_type = SurfaceType::Gravel;
    point.curvature = 0.0;
    point.banking = 0.0;
    point.has_box_lane = false;
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

  pit_box_positions_ = {1000.0, 1150.0, 1300.0, 1450.0};
}

// Build a default closed-loop track composed of:
//   - straight section (0..25%)
//   - right-hand corner (25..50%)
//   - straight section (50..75%)
//   - left-hand corner (75..100%)
void Track::build_default_track() {
  points_.clear();
  const double straightLength = 765.0;
  const double curveRadius = 75.0;
  const int segmentsPerStraight = 150;
  const int segmentsPerCurve = 75;

  // Segment 1: straight section along the +x axis (0% .. 25%).
  // Box lane runs parallel on the left side of the track.
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
    point.surface_type = SurfaceType::Asphalt;
    point.curvature = 0.0;
    point.banking = 0.0;
    point.has_box_lane = true;
    point.box_lane_width = 3.5;
    points_.push_back(point);
  }

  // Segment 2: right-hand (clockwise) corner (25% .. 50%).
  for (int i = 1; i <= segmentsPerCurve; ++i) {
    double t = static_cast<double>(i) / segmentsPerCurve;
    double angle = -kHalfPi + t * kPi;
    Vec2 pos(straightLength + curveRadius * std::cos(angle), curveRadius + curveRadius * std::sin(angle));
    Vec2 tangent(-std::sin(angle), std::cos(angle));
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.surface_type = SurfaceType::Asphalt;
    point.curvature = 0.0;
    point.banking = 0.0;
    points_.push_back(point);
  }

  // Segment 3: straight section heading back along -x (50% .. 75%).
  for (int i = 1; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    Vec2 pos(straightLength - t * straightLength, 2.0 * curveRadius);
    Vec2 tangent(-1.0, 0.0);
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.surface_type = SurfaceType::OldAsphalt;
    point.curvature = 0.0;
    point.banking = 0.0;
    points_.push_back(point);
  }

  // Segment 4: left-hand (counter-clockwise) corner closing the loop (75% .. 100%).
  for (int i = 1; i <= segmentsPerCurve; ++i) {
    double t = static_cast<double>(i) / segmentsPerCurve;
    double angle = kHalfPi + t * kPi;
    Vec2 pos(curveRadius * std::cos(angle), curveRadius + curveRadius * std::sin(angle));
    Vec2 tangent(-std::sin(angle), std::cos(angle));
    tangent.normalize();
    Vec2 normal(-tangent.y(), tangent.x());
    TrackPoint point;
    point.position = pos;
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.surface_type = SurfaceType::Gravel;
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

bool Track::has_box_lane_at(double distance) const {
  const auto tp = at(distance);
  return tp.has_box_lane;
}

double Track::box_lane_width_at(double distance) const {
  const auto tp = at(distance);
  return tp.box_lane_width;
}

SurfaceType Track::surface_type_at(double distance) const {
  if (points_.empty()) return default_surface_;

  distance = std::fmod(distance, total_length_);
  if (distance < 0.0) distance += total_length_;

  const double step = total_length_ / (points_.size() - 1);
  const double raw_index = distance / step;
  const int index = static_cast<int>(std::floor(raw_index));

  const int i0 = std::clamp(index, 0, static_cast<int>(points_.size()) - 1);
  const int i1 = std::clamp(index + 1, 0, static_cast<int>(points_.size()) - 1);

  const double frac = raw_index - index;
  return (frac < 0.5) ? points_[i0].surface_type : points_[i1].surface_type;
}

void Track::set_surface_at(double distance, SurfaceType type) {
  if (points_.empty()) return;

  distance = std::fmod(distance, total_length_);
  if (distance < 0.0) distance += total_length_;

  const double step = total_length_ / (points_.size() - 1);
  const double raw_index = distance / step;
  const int index = static_cast<int>(std::floor(raw_index));

  const int i0 = std::clamp(index, 0, static_cast<int>(points_.size()) - 1);
  const int i1 = std::clamp(index + 1, 0, static_cast<int>(points_.size()) - 1);

  const double frac = raw_index - index;
  const int target = (frac < 0.5) ? i0 : i1;
  points_[target].surface_type = type;
  points_[target].friction = friction_for_surface(type);
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
  result.surface_type = (frac < 0.5) ? points_[i0].surface_type : points_[i1].surface_type;
  result.distance = distance;
  result.has_box_lane = points_[i0].has_box_lane && points_[i1].has_box_lane;
  result.box_lane_width = lerp(points_[i0].box_lane_width, points_[i1].box_lane_width, frac);

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
