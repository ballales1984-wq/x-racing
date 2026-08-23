#pragma once

#include "common.h"

namespace p0::track {

struct TrackPoint {
  Vec2 position;
  Vec2 tangent;
  Vec2 normal;
  double curvature = 0.0;
  double width = 12.0;
  double banking = 0.0;
  double friction = 1.0;
  double distance = 0.0;
};

struct TrackParams {
  double total_length = 5000.0;
  double default_width = 12.0;
  double default_friction = 1.0;
};

class Track {
 public:
  explicit Track(const TrackParams& params = {});
  ~Track() = default;

  TrackPoint at(double distance) const;
  Vec2 get_start_position() const;
  double get_start_heading() const;
  double length() const { return total_length_; }

 private:
  void build_default_track();
  TrackPoint interpolate(double distance) const;

  std::vector<TrackPoint> points_;
  double total_length_ = 0.0;
  double default_width_ = 12.0;
  double default_friction_ = 1.0;
};

}
