#include "track/track.h"
#include "physics/types.h"
#include <algorithm>
#include <cmath>
#include <numeric>

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
  } else if (type == TrackType::CustomCircuit) {
    build_custom_track();
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

// Build a custom clockwise road course with a clear pit lane on the back straight.
//   - main straight heading east (0 .. 450 m): NO box lane
//   - right hairpin (450 .. ~858 m), center (450, -130), radius 130 m, clockwise
//   - pit straight heading west (~858 .. 1258 m): BOX LANE on right side
//   - left hairpin (~1258 .. ~1667 m), center (50, -130), radius 130 m, counter-clockwise
// Start/finish is at (0, 0) heading east.
void Track::build_custom_track() {
  points_.clear();
  pit_box_positions_.clear();
  const double L1 = 450.0;
  const double R = 130.0;
  const double L2 = 450.0;
  const int segmentsPerStraight = 120;
  const int segmentsPerCurve = 60;

  Vec2 tangent(1.0, 0.0);
  Vec2 normal(-tangent.y(), tangent.x());

  // Section 1: main straight heading east (0 .. 450 m). NO box lane.
  for (int i = 0; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    TrackPoint point;
    point.position = tangent * (t * L1);
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

  // Section 2: right hairpin, center (L1, -R), clockwise.
  Vec2 cornerCenter2(L1, -R);
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

  // Section 3: pit straight heading west. BOX LANE on right side of travel.
  Vec2 pos3(L1, -2.0 * R);
  Vec2 tangent3(-1.0, 0.0);
  Vec2 normal3(-tangent3.y(), tangent3.x());
  for (int i = 1; i <= segmentsPerStraight; ++i) {
    double t = static_cast<double>(i) / segmentsPerStraight;
    TrackPoint point;
    point.position = pos3 + tangent3 * (t * L2);
    point.tangent = tangent3;
    point.normal = normal3;
    point.width = default_width_;
    point.friction = default_friction_ * 0.95;
    point.surface_type = SurfaceType::Asphalt;
    point.curvature = 0.0;
    point.banking = 0.0;
    point.has_box_lane = true;
    point.box_lane_width = 4.0;
    points_.push_back(point);
  }

  // Section 4: left hairpin, center (L1 - L2, -R), counter-clockwise.
  Vec2 cornerCenter4(L1 - L2, -R);
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
    point.surface_type = SurfaceType::Gravel;
    point.curvature = 1.0 / R;
    point.banking = -0.02;
    point.has_box_lane = false;
    points_.push_back(point);
  }

  pit_box_positions_ = {900.0, 1000.0, 1100.0, 1200.0};

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

GridDefinition Track::generate_grid(GridLayout layout, int slot_count) const {
  GridDefinition def;
  def.layout = layout;
  def.max_slots = 30;
  def.row_spacing = 8.0;
  def.column_spacing = 6.0;

  if (points_.empty() || slot_count <= 0) return def;

  const auto start_tp = at(0.0);
  const Vec2 start_pos = start_tp.position;
  const Vec2 tangent = start_tp.tangent.normalized();
  const Vec2 normal = start_tp.normal.normalized();

  const double track_width = start_tp.width;
  const double car_width = 4.0;
  const double car_depth = 10.0;
  const double edge_gap = 1.0;

  switch (layout) {
    case GridLayout::SINGLE_COLUMN: {
      for (int i = 0; i < slot_count; ++i) {
        GridSlot slot;
        slot.slot_id = i + 1;
        const double longitudinal = car_depth * 0.5 + i * (car_depth + def.row_spacing);
        slot.transform.position = start_pos - tangent * longitudinal;
        slot.transform.forward = tangent;
        slot.width = car_width;
        slot.depth = car_depth;
        def.slots.push_back(slot);
      }
      break;
    }
    case GridLayout::TWO_COLUMN: {
      for (int i = 0; i < slot_count; ++i) {
        GridSlot slot;
        slot.slot_id = i + 1;

        const int row = i / 2;
        const bool is_odd = (i % 2 == 0);

        double longitudinal = car_depth * 0.5 + row * (car_depth + def.row_spacing);

        if (row == 0 && is_odd) {
          longitudinal = car_depth * 0.25;
        }

        const double max_lateral = (track_width * 0.5) - (car_width * 0.5) - edge_gap;

        double avg_curvature = 0.0;
        int samples = 0;
        for (double d = 2.0; d < 80.0 && d < total_length_; d += 5.0) {
          avg_curvature += at(d).curvature;
          samples++;
        }
        if (samples > 0) avg_curvature /= samples;

        const bool pole_left = avg_curvature >= 0.0;
        const bool left_side = is_odd ? pole_left : !pole_left;

        const double lateral = left_side ? max_lateral : -max_lateral;

        slot.transform.position = start_pos - tangent * longitudinal + normal * lateral;
        slot.transform.forward = tangent;
        slot.width = car_width;
        slot.depth = car_depth;
        def.slots.push_back(slot);
      }
      break;
    }
    default:
      break;
  }

  return def;
}

PitLaneDefinition Track::build_pit_lane_definition() const {
  PitLaneDefinition def;
  if (points_.empty()) return def;

  int first_box = -1;
  int last_box = -1;
  for (int i = 0; i < (int)points_.size(); ++i) {
    if (points_[i].has_box_lane) {
      if (first_box < 0) first_box = i;
      last_box = i;
    }
  }

  if (first_box < 0 || last_box < 0) return def;

  const auto& entry_tp = points_[first_box];
  const auto& exit_tp = points_[last_box];

  def.entry.transform.position = entry_tp.position + entry_tp.normal * (entry_tp.width * 0.5 + entry_tp.box_lane_width * 0.5);
  def.entry.transform.forward = entry_tp.tangent;
  def.entry.width = entry_tp.box_lane_width;

  def.exit.transform.position = exit_tp.position + exit_tp.normal * (exit_tp.width * 0.5 + exit_tp.box_lane_width * 0.5);
  def.exit.transform.forward = exit_tp.tangent;
  def.exit.width = exit_tp.box_lane_width;

  const double step = 5.0;
  for (double d = entry_tp.distance; d <= exit_tp.distance + kEpsilon; d += step) {
    double dd = std::fmod(d, total_length_);
    if (dd < 0.0) dd += total_length_;
    const auto tp = at(dd);
    PitLanePathPoint pp;
    pp.transform.position = tp.position + tp.normal * (tp.width * 0.5 + tp.box_lane_width * 0.5);
    pp.transform.forward = tp.tangent;
    pp.width = tp.box_lane_width;
    def.path.push_back(pp);
  }

  for (double box_pos : pit_box_positions_) {
    const auto tp = at(box_pos);
    PitBox box;
    box.box_id = (int)def.boxes.size();
    box.team_id = 0;
    box.position.position = tp.position + tp.normal * (tp.width * 0.5 + tp.box_lane_width * 0.5 + 2.0);
    box.position.forward = tp.tangent;
    box.service_position = box.position;
    box.entry_direction = tp.tangent;
    box.exit_direction = tp.tangent;
    box.width = 4.0;
    box.depth = 6.0;
    box.state = p0::race::BoxState::FREE;
    def.boxes.push_back(box);
  }

  def.speed_zone.start_line.position = def.entry.transform.position;
  def.speed_zone.start_line.forward = def.entry.transform.forward;
  def.speed_zone.end_line.position = def.exit.transform.position;
  def.speed_zone.end_line.forward = def.exit.transform.forward;
  def.speed_zone.speed_limit_m_s = 16.67;
  def.speed_zone.tolerance_m_s = 1.39;
  def.speed_zone.detection_mode = p0::race::SpeedDetectionMode::AVERAGE_SPEED;
  def.speed_zone.violation_type = p0::race::ViolationType::PIT_SPEED_EXCEEDED;
  def.speed_zone.penalty = p0::race::PenaltyType::DRIVE_THROUGH;

  def.merge_zone.start = def.exit.transform;
  def.merge_zone.end.position = exit_tp.position;
  def.merge_zone.end.forward = exit_tp.tangent;

  def.speed_limit_m_s = 16.67;
  def.pit_lane_length_m = exit_tp.distance - entry_tp.distance;

  return def;
}

Track::TrackMesh Track::generate_mesh() const {
  if (mesh_cache_valid_) return mesh_cache_;

  TrackMesh& mesh = mesh_cache_;
  if (points_.size() < 2) {
    mesh_cache_valid_ = true;
    return mesh;
  }

  mesh.vertices.reserve(points_.size() * 2);
  mesh.indices.reserve(points_.size() * 6);

  for (const auto& tp : points_) {
    const double half_width = tp.width * 0.5;
    mesh.vertices.push_back(tp.position + tp.normal * half_width);
    mesh.vertices.push_back(tp.position - tp.normal * half_width);
  }

  const int n = static_cast<int>(points_.size());
  for (int i = 0; i < n; ++i) {
    const int next = (i + 1) % n;
    const int bl = i * 2;
    const int br = i * 2 + 1;
    const int tl = next * 2;
    const int tr = next * 2 + 1;

    mesh.indices.push_back(bl);
    mesh.indices.push_back(tl);
    mesh.indices.push_back(br);

    mesh.indices.push_back(br);
    mesh.indices.push_back(tl);
    mesh.indices.push_back(tr);
  }

  mesh_cache_valid_ = true;
  return mesh;
}

bool Track::collides_with_mesh(const Vec2& point) const {
  const TrackMesh& mesh = generate_mesh();
  if (mesh.vertices.size() < 3) return false;

  for (size_t i = 0; i < mesh.indices.size(); i += 3) {
    const Vec2& a = mesh.vertices[mesh.indices[i]];
    const Vec2& b = mesh.vertices[mesh.indices[i + 1]];
    const Vec2& c = mesh.vertices[mesh.indices[i + 2]];

    if (point_in_triangle(point, a, b, c)) return true;
  }

  return false;
}

}

