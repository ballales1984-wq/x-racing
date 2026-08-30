#pragma once

#include "common.h"
#include "track/track.h"
#include <vector>

namespace p0::ui {

struct TrackMapConfig {
  int width = 200;
  int height = 200;
  int margin = 20;
  double track_color[3] = {0.5, 0.5, 0.5};
  double car_color[3] = {0.0, 1.0, 0.0};
  double opponent_color[3] = {1.0, 0.0, 0.0};
  double player_color[3] = {0.0, 0.0, 1.0};
};

struct CarPosition {
  int car_id = -1;
  double distance_along_track = 0.0;
  bool is_player = false;
};

class TrackMap {
 public:
  TrackMap();

  void set_track(const track::Track* track);
  void set_config(const TrackMapConfig& config);
  const TrackMapConfig& config() const { return config_; }

  void update(const std::vector<CarPosition>& cars);
  const std::vector<CarPosition>& cars() const { return cars_; }

  Vec2 world_to_map(const Vec2& world_pos) const;
  Vec2 track_to_map(double distance_along_track) const;

  double map_width() const { return config_.width; }
  double map_height() const { return config_.height; }

  void compute_bounds();
  double min_x() const { return min_x_; }
  double max_x() const { return max_x_; }
  double min_y() const { return min_y_; }
  double max_y() const { return max_y_; }

 private:
  const track::Track* track_ = nullptr;
  TrackMapConfig config_;
  std::vector<CarPosition> cars_;
  double min_x_ = 0.0;
  double max_x_ = 1.0;
  double min_y_ = 0.0;
  double max_y_ = 1.0;
  bool bounds_valid_ = false;
};

}
