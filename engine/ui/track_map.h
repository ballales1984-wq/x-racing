// Project 0 — 2D track minimap (world-to-screen projection)
// Namespace: p0::ui
#pragma once

#include "common.h"
#include "track/track.h"
#include <vector>

namespace p0::ui {

// Visual configuration for the minimap rendering (colors, size, margins).
struct TrackMapConfig {
  int width = 200;  // pixels
  int height = 200;  // pixels
  int margin = 20;  // pixels, padding inside the minimap box
  double track_color[3] = {0.5, 0.5, 0.5};  // RGB [0,1]
  double car_color[3] = {0.0, 1.0, 0.0};  // RGB [0,1]
  double opponent_color[3] = {1.0, 0.0, 0.0};  // RGB [0,1]
  double player_color[3] = {0.0, 0.0, 1.0};  // RGB [0,1]
};

// Snapshot of a car's position for minimap rendering.
struct CarPosition {
  int car_id = -1;
  double distance_along_track = 0.0;  // m
  bool is_player = false;
};

// Projects track geometry and car positions onto a 2D minimap canvas.
// Computes world bounds and converts track distance / world coords to pixel coords.
class TrackMap {
 public:
  TrackMap();

  // Bind the track geometry used for projection.
  void set_track(const track::Track* track);
  void set_config(const TrackMapConfig& config);
  const TrackMapConfig& config() const { return config_; }

  // Update car positions for the current frame.
  void update(const std::vector<CarPosition>& cars);
  const std::vector<CarPosition>& cars() const { return cars_; }

  // Project world coordinates to minimap pixel coordinates.
  Vec2 world_to_map(const Vec2& world_pos) const;
  // Project track distance to minimap pixel coordinates.
  Vec2 track_to_map(double distance_along_track) const;

  double map_width() const { return config_.width; }
  double map_height() const { return config_.height; }

  // Recompute the world-space bounding box of the track.
  void compute_bounds();
  double min_x() const { return min_x_; }
  double max_x() const { return max_x_; }
  double min_y() const { return min_y_; }
  double max_y() const { return max_y_; }

  private:
   const track::Track* track_ = nullptr;
   TrackMapConfig config_;
   std::vector<CarPosition> cars_;
   // --- World bounds (recomputed by compute_bounds) ---
   double min_x_ = 0.0;
   double max_x_ = 1.0;
   double min_y_ = 0.0;
   double max_y_ = 1.0;
   bool bounds_valid_ = false;
};

}
