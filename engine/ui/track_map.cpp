#include "ui/track_map.h"
#include <algorithm>
#include <cmath>

namespace p0::ui {

TrackMap::TrackMap() = default;

void TrackMap::set_track(const track::Track* track) {
  track_ = track;
  bounds_valid_ = false;
}

void TrackMap::set_config(const TrackMapConfig& config) {
  config_ = config;
}

void TrackMap::update(const std::vector<CarPosition>& cars) {
  cars_ = cars;
}

Vec2 TrackMap::world_to_map(const Vec2& world_pos) const {
  if (!bounds_valid_) return Vec2();
  double range_x = max_x_ - min_x_;
  double range_y = max_y_ - min_y_;
  if (range_x < 0.001) range_x = 1.0;
  if (range_y < 0.001) range_y = 1.0;

  double map_area_width = config_.width - 2 * config_.margin;
  double map_area_height = config_.height - 2 * config_.margin;

  double norm_x = (world_pos.x() - min_x_) / range_x;
  double norm_y = (world_pos.y() - min_y_) / range_y;

  double map_x = config_.margin + norm_x * map_area_width;
  double map_y = config_.margin + (1.0 - norm_y) * map_area_height;

  return Vec2(static_cast<float>(map_x), static_cast<float>(map_y));
}

Vec2 TrackMap::track_to_map(double distance_along_track) const {
  if (!track_) return Vec2();
  const auto& tp = track_->at(distance_along_track);
  return world_to_map(tp.position);
}

void TrackMap::compute_bounds() {
  if (!track_ || track_->length() <= 0.0) {
    bounds_valid_ = false;
    return;
  }

  min_x_ = 1e9;
  max_x_ = -1e9;
  min_y_ = 1e9;
  max_y_ = -1e9;

  for (double d = 0.0; d < track_->length(); d += 5.0) {
    const auto& tp = track_->at(d);
    min_x_ = std::min(min_x_, tp.position.x());
    max_x_ = std::max(max_x_, tp.position.x());
    min_y_ = std::min(min_y_, tp.position.y());
    max_y_ = std::max(max_y_, tp.position.y());
  }

  double range_x = max_x_ - min_x_;
  double range_y = max_y_ - min_y_;
  double center_x = (min_x_ + max_x_) * 0.5;
  double center_y = (min_y_ + max_y_) * 0.5;
  double max_range = std::max(range_x, range_y) * 0.6;

  min_x_ = center_x - max_range;
  max_x_ = center_x + max_range;
  min_y_ = center_y - max_range;
  max_y_ = center_y + max_range;

  bounds_valid_ = true;
}

}
