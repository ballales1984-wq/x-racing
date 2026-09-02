// Project 0 — Racing line optimizer (geometric + speed-based)
// Namespace: p0::ai
#pragma once

#include "common.h"
#include "track/track.h"
#include "track/track_data.h"
#include <vector>

namespace p0::ai {

// Single point on the computed racing line.
// Contains position, tangent, lateral offset from track center, target speed, and curvature.
struct RacingLinePoint {
  Vec2 position;
  Vec2 tangent;
  double lateral_offset = 0.0;  // m, positive = outside
  double speed_m_s = 80.0;
  double curvature = 0.0;  // 1/m
  double distance = 0.0;  // m along track
};

// Computes a simplified racing line from track geometry.
// Uses a geometric model: wider line through corners (optimal_offset)
// and apex speed estimation from curvature and friction.
class RacingLineOptimizer {
 public:
  explicit RacingLineOptimizer(const track::Track& track);

  const std::vector<RacingLinePoint>& points() const { return points_; }

  // Convert to the format consumed by the track system.
  std::vector<track::RacingLineSample> to_racing_line_samples() const;

  // Interpolate target speed at a given track distance.
  double target_speed_at(double distance) const;

 private:
  const track::Track* track_ = nullptr;
  std::vector<RacingLinePoint> points_;

  void compute();
  // Geometric optimal lateral offset for a given curvature and track width.
  double optimal_offset(double curvature, double width) const;
  // Estimated apex speed from curvature and friction coefficient.
  double apex_speed(double curvature, double friction) const;
};

}
