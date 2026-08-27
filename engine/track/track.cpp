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

// Build a default closed-loop oval track.
//   - straight east: (0,0) -> (L,0)
//   - right semicircle: center (L,-R), radius R
//   - straight west: (L,-2R) -> (0,-2R)
//   - left semicircle: center (0,-R), radius R
void Track::build_default_track() {
  points_.clear();
  const double L = 300.0;
  const double R = 100.0;
  const int segmentsPerStraight = 120;
  const int segmentsPerCurve = 60;

  Vec2 tangent(1.0, 0.0);
  Vec2 normal(-tangent.y(), tangent.x());

  // Segment 1: straight east (0,0) -> (L,0).
  // The box lane is active on the middle 80% of this straight.
  for (int i = 0; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    TrackPoint point;
    point.position = tangent * (t * L);
    point.tangent = tangent;
    point.normal = normal;
    point.width = default_width_;
    point.friction = default_friction_;
    point.surface_type = SurfaceType::Asphalt;
    point.curvature = 0.0;
    point.banking = 0.0;
    point.has_box_lane = (i > segmentsPerStraight * 0.1 && i < segmentsPerStraight * 0.9);
    point.box_lane_width = 3.5;
    points_.push_back(point);
  }

  // Segment 2: right semicircle, center (L, -R), clockwise from (L,0) to (L,-2R).
  // Curvature is negative for a right-hand turn in our coordinate system.
  Vec2 cornerCenter2(L, -R);
  for (int i = 1; i <= segmentsPerCurve; ++i) {
    double t = static_cast<double>(i) / segmentsPerCurve;
    double angle = kHalfPi - t * kPi;
    Vec2 p = cornerCenter2 + Vec2(R * std::cos(angle), R * std::sin(angle));
    Vec2 tang(-std::sin(angle), std::cos(angle));
    tang.normalize();
    Vec2 norm(-tang.y(), tang.x());
    TrackPoint point;
    point.position = p;
    point.tangent = tang;
    point.normal = norm;
    point.width = default_width_;
    point.friction = default_friction_ * 0.98;
    point.surface_type = SurfaceType::OldAsphalt;
    point.curvature = -1.0 / R;
    point.banking = 0.02;
    point.has_box_lane = false;
    points_.push_back(point);
  }

  // Segment 3: straight west (L,-2R) -> (0,-2R).
  Vec2 pos3(L, -2.0 * R);
  Vec2 tangent3(-1.0, 0.0);
  Vec2 normal3(-tangent3.y(), tangent3.x());
  for (int i = 1; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    TrackPoint point;
    point.position = pos3 + tangent3 * (t * L);
    point.tangent = tangent3;
    point.normal = normal3;
    point.width = default_width_;
    point.friction = default_friction_;
    point.surface_type = SurfaceType::Asphalt;
    point.curvature = 0.0;
    point.banking = 0.0;
    point.has_box_lane = false;
    points_.push_back(point);
  }

  // Segment 4: left semicircle, center (0, -R), counter-clockwise from (0,-2R) to (0,0).
  Vec2 cornerCenter4(0.0, -R);
  for (int i = 1; i <= segmentsPerCurve; ++i) {
    double t = static_cast<double>(i) / segmentsPerCurve;
    double angle = -kHalfPi - t * kPi;
    Vec2 p = cornerCenter4 + Vec2(R * std::cos(angle), R * std::sin(angle));
    Vec2 tang(-std::sin(angle), std::cos(angle));
    tang.normalize();
    Vec2 norm(-tang.y(), tang.x());
    TrackPoint point;
    point.position = p;
    point.tangent = tang;
    point.normal = norm;
    point.width = default_width_;
    point.friction = default_friction_ * 0.97;
    point.surface_type = SurfaceType::Asphalt;
    point.curvature = 1.0 / R;
    point.banking = 0.01;
    point.has_box_lane = false;
    points_.push_back(point);
  }

  // Compute cumulative arc-length distance for each sampled point.
  for (size_t i = 0; i < points_.size(); ++i) {
    if (i == 0) {
      points_[i].distance = 0.0;
    } else {
      double segmentLength = (points_[i].position - points_[i - 1].position).norm();
      points_[i].distance = points_[i - 1].distance + segmentLength;
    }
  }
  total_length_ = points_.back().distance;

  pit_box_positions_ = {50.0, 100.0, 150.0, 200.0, 250.0};

  // Compute local curvature at each point from the turning angle between
  // adjacent segments (finite-difference approximation).
  for (size_t i = 0; i < points_.size(); ++i) {
    const auto& prev = points_[(i > 0) ? i - 1 : points_.size() - 2];
    const auto& next = points_[(i + 1 < points_.size()) ? i + 1 : 0];
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

// Build a pit-circuit track composed of:
//   - main straight (0 .. 500 m)
//   - right-hand corner (500 .. ~800 m)
//   - pit-lane straight with pit boxes (~800 .. ~1300 m)
//   - left-hand hairpin closing the loop (~1300 .. ~1600 m)
void Track::build_pit_track() {
  points_.clear();
  pit_box_positions_.clear();
  const double L = 500.0;
  const double R = 90.0;
  const int segmentsPerStraight = 120;
  const int segmentsPerCurve = 60;

  Vec2 tangent(1.0, 0.0);
  Vec2 normal(-tangent.y(), tangent.x());

  // Section 1: main straight heading east (0 .. 500 m).
  for (int i = 0; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    TrackPoint point;
    point.position = tangent * (t * L);
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

  // Section 2: right-hand semicircle, center (L, -R), clockwise.
  Vec2 cornerCenter2(L, -R);
  for (int i = 1; i <= segmentsPerCurve; ++i) {
    double t = static_cast<double>(i) / segmentsPerCurve;
    double angle = kHalfPi - t * kPi;
    Vec2 p = cornerCenter2 + Vec2(R * std::cos(angle), R * std::sin(angle));
    Vec2 tang(-std::sin(angle), std::cos(angle));
    tang.normalize();
    Vec2 norm(-tang.y(), tang.x());
    TrackPoint point;
    point.position = p;
    point.tangent = tang;
    point.normal = norm;
    point.width = default_width_;
    point.friction = default_friction_ * 0.95;
    point.surface_type = SurfaceType::OldAsphalt;
    point.curvature = -1.0 / R;
    point.banking = 0.02;
    point.has_box_lane = false;
    points_.push_back(point);
  }

  // Section 3: pit-lane straight heading west.
  Vec2 pos3(L, -2.0 * R);
  Vec2 tangent3(-1.0, 0.0);
  Vec2 normal3(-tangent3.y(), tangent3.x());
  for (int i = 1; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    TrackPoint point;
    point.position = pos3 + tangent3 * (t * L);
    point.tangent = tangent3;
    point.normal = normal3;
    point.width = default_width_;
    point.friction = default_friction_ * 0.9;
    point.surface_type = SurfaceType::Asphalt;
    point.curvature = 0.0;
    point.banking = 0.0;
    point.has_box_lane = true;
    point.box_lane_width = 4.0;
    points_.push_back(point);
  }

  // Section 4: left-hand semicircle, center (0, -R), counter-clockwise.
  Vec2 cornerCenter4(0.0, -R);
  for (int i = 1; i <= segmentsPerCurve; ++i) {
    double t = static_cast<double>(i) / segmentsPerCurve;
    double angle = -kHalfPi - t * kPi;
    Vec2 p = cornerCenter4 + Vec2(R * std::cos(angle), R * std::sin(angle));
    Vec2 tang(-std::sin(angle), std::cos(angle));
    tang.normalize();
    Vec2 norm(-tang.y(), tang.x());
    TrackPoint point;
    point.position = p;
    point.tangent = tang;
    point.normal = norm;
    point.width = default_width_;
    point.friction = default_friction_;
    point.surface_type = SurfaceType::Gravel;
    point.curvature = 1.0 / R;
    point.banking = -0.02;
    point.has_box_lane = false;
    points_.push_back(point);
  }

  pit_box_positions_ = {850.0, 950.0, 1050.0, 1150.0};

  // Compute cumulative arc-length distance for each sampled point.
  for (size_t i = 0; i < points_.size(); ++i) {
    if (i == 0) {
      points_[i].distance = 0.0;
    } else {
      double segmentLength = (points_[i].position - points_[i - 1].position).norm();
      points_[i].distance = points_[i - 1].distance + segmentLength;
    }
  }
  total_length_ = points_.back().distance;

  // Compute local curvature at each point from the turning angle between
  // adjacent segments (finite-difference approximation).
  for (size_t i = 0; i < points_.size(); ++i) {
    const auto& prev = points_[(i > 0) ? i - 1 : points_.size() - 2];
    const auto& next = points_[(i + 1 < points_.size()) ? i + 1 : 0];
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

// Find the two adjacent points surrounding a given distance along the track.
// Uses binary search on the cumulative distance array for accuracy regardless
// of non-uniform point spacing.
void Track::find_adjacent_points(double distance, int& i0, int& i1, double& frac) const {
  if (points_.size() < 2) {
    i0 = i1 = 0;
    frac = 0.0;
    return;
  }

  // Binary search for the first point with distance >= target
  int lo = 0;
  int hi = static_cast<int>(points_.size()) - 1;
  while (lo < hi) {
    const int mid = lo + (hi - lo) / 2;
    if (points_[mid].distance < distance) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }

  i0 = std::max(lo - 1, 0);
  i1 = std::min(lo, static_cast<int>(points_.size()) - 1);

  if (i0 == i1) {
    frac = 0.0;
    return;
  }

  const double seg_len = points_[i1].distance - points_[i0].distance;
  if (seg_len > kEpsilon) {
    frac = (distance - points_[i0].distance) / seg_len;
  } else {
    frac = 0.0;
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

  // Handle exact end-of-track case
  if (distance >= points_.back().distance - kEpsilon) {
    return points_.back();
  }

  int i0, i1;
  double frac;
  find_adjacent_points(distance, i0, i1, frac);
  return interpolate(distance, i0, i1, frac);
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

  if (distance >= points_.back().distance - kEpsilon) {
    return points_.back().surface_type;
  }

  int i0, i1;
  double frac;
  find_adjacent_points(distance, i0, i1, frac);

  return (frac < 0.5) ? points_[i0].surface_type : points_[i1].surface_type;
}

void Track::set_surface_at(double distance, SurfaceType type) {
  if (points_.empty()) return;

  distance = std::fmod(distance, total_length_);
  if (distance < 0.0) distance += total_length_;

  if (distance >= points_.back().distance - kEpsilon) {
    points_.back().surface_type = type;
    points_.back().friction = friction_for_surface(type);
    return;
  }

  int i0, i1;
  double frac;
  find_adjacent_points(distance, i0, i1, frac);

  const int target = (frac < 0.5) ? i0 : i1;
  points_[target].surface_type = type;
  points_[target].friction = friction_for_surface(type);
}

// Linear interpolation between two adjacent track points
TrackPoint Track::interpolate(double distance, int i0, int i1, double frac) const {
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
