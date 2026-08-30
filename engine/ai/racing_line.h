#pragma once

#include "common.h"
#include "track/track.h"
#include "track/track_data.h"
#include <vector>

namespace p0::ai {

struct RacingLinePoint {
  Vec2 position;
  Vec2 tangent;
  double lateral_offset = 0.0;
  double speed_m_s = 80.0;
  double curvature = 0.0;
  double distance = 0.0;
};

class RacingLineOptimizer {
 public:
  explicit RacingLineOptimizer(const track::Track& track);

  const std::vector<RacingLinePoint>& points() const { return points_; }

  std::vector<track::RacingLineSample> to_racing_line_samples() const;

  double target_speed_at(double distance) const;

 private:
  const track::Track* track_ = nullptr;
  std::vector<RacingLinePoint> points_;

  void compute();
  double optimal_offset(double curvature, double width) const;
  double apex_speed(double curvature, double friction) const;
};

}
