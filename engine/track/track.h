#pragma once

#include "common.h"

// Project 0 — parametric track model
// Namespace: p0::track
namespace p0::track {

// Single sample point along the track centerline
struct TrackPoint {
  Vec2 position;                         // m, world position
  Vec2 tangent;                          // unit vector, forward direction
  Vec2 normal;                           // unit vector, leftward direction
  double curvature = 0.0;                // 1/m, signed curvature
  double width = 12.0;                   // m, track width at this point
  double banking = 0.0;                  // rad, banking angle
  double friction = 1.0;                 // [-], local friction modifier
  double distance = 0.0;                 // m, cumulative distance from start

  bool has_box_lane = false;             // true if a box/pit lane exists here
  double box_lane_width = 3.5;           // m, width of the box lane
};

// Parameters for track generation
struct TrackParams {
  double total_length = 5000.0;          // m, closed-loop length
  double default_width = 12.0;           // m
  double default_friction = 1.0;         // [-]
};

// Parametric closed-loop track.
// The track is built from a sequence of precomputed points.
// Querying at any distance returns interpolated geometry.
class Track {
 public:
  explicit Track(const TrackParams& params = {});
  ~Track() = default;

  // Get interpolated track data at distance d (wraps around loop)
  TrackPoint at(double distance) const;
  // Starting position and heading for a new lap
  Vec2 get_start_position() const;
  double get_start_heading() const;
  double length() const { return total_length_; }

  bool has_box_lane_at(double distance) const;
  double box_lane_width_at(double distance) const;

 private:
  void build_default_track();
  TrackPoint interpolate(double distance) const;

  std::vector<TrackPoint> points_;
  double total_length_ = 0.0;
  double default_width_ = 12.0;
  double default_friction_ = 1.0;
};

}
