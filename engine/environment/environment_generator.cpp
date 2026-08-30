#include "environment/environment_generator.h"
#include <algorithm>
#include <cmath>

namespace p0::environment {

EnvironmentGenerator::EnvironmentGenerator() {
  std::random_device rd;
  rng_.seed(rd());
}

void EnvironmentGenerator::set_track(const track::Track* track) {
  track_ = track;
}

void EnvironmentGenerator::set_config(const EnvironmentConfig& config) {
  config_ = config;
  rng_.seed(config.seed);
}

void EnvironmentGenerator::generate() {
  clear();
  if (!track_) return;

  double track_len = track_->length();
  if (track_len <= 0.0) return;

  std::uniform_real_distribution<double> rot_dist(0.0, 360.0);
  std::uniform_real_distribution<double> scale_dist(0.7, 1.3);
  std::uniform_int_distribution<int> variant_dist(0, 4);
  std::uniform_real_distribution<double> prob_dist(0.0, 1.0);

  double step = 5.0;
  for (double d = 0.0; d < track_len; d += step) {
    const auto& tp = track_->at(d);
    Vec2 left_pos = tp.position + tp.normal * (config_.tree_zone.min_distance_from_center + 5.0);
    Vec2 right_pos = tp.position - tp.normal * (config_.tree_zone.min_distance_from_center + 5.0);

    if (prob_dist(rng_) < config_.tree_zone.density && static_cast<int>(objects_.size()) < config_.tree_zone.max_objects) {
      EnvironmentObject obj;
      obj.id = next_id_++;
      obj.type = EnvironmentObjectType::TREE;
      obj.position = left_pos;
      obj.rotation = rot_dist(rng_);
      obj.scale = scale_dist(rng_);
      obj.variant = variant_dist(rng_);
      objects_.push_back(obj);
    }

    if (prob_dist(rng_) < config_.tree_zone.density * 0.7 && static_cast<int>(objects_.size()) < config_.tree_zone.max_objects) {
      EnvironmentObject obj;
      obj.id = next_id_++;
      obj.type = EnvironmentObjectType::TREE;
      obj.position = right_pos;
      obj.rotation = rot_dist(rng_);
      obj.scale = scale_dist(rng_);
      obj.variant = variant_dist(rng_);
      objects_.push_back(obj);
    }

    if (prob_dist(rng_) < config_.rock_zone.density) {
      EnvironmentObject obj;
      obj.id = next_id_++;
      obj.type = EnvironmentObjectType::ROCK;
      obj.position = (prob_dist(rng_) < 0.5) ? left_pos : right_pos;
      obj.rotation = rot_dist(rng_);
      obj.scale = scale_dist(rng_) * 0.5;
      obj.variant = variant_dist(rng_) % 3;
      objects_.push_back(obj);
    }

    if (prob_dist(rng_) < config_.grass_zone.density) {
      EnvironmentObject obj;
      obj.id = next_id_++;
      obj.type = EnvironmentObjectType::GRASS;
      obj.position = (prob_dist(rng_) < 0.5) ? left_pos : right_pos;
      obj.rotation = rot_dist(rng_);
      obj.scale = scale_dist(rng_) * 0.3;
      obj.variant = variant_dist(rng_) % 2;
      objects_.push_back(obj);
    }
  }

  if (config_.place_barriers) {
    for (double d = 0.0; d < track_len; d += 8.0) {
      const auto& tp = track_->at(d);
      double track_width = 7.5;

      EnvironmentObject left_barrier;
      left_barrier.id = next_id_++;
      left_barrier.type = EnvironmentObjectType::BARRIER;
      left_barrier.position = tp.position + tp.normal * (track_width + 1.0);
      left_barrier.rotation = std::atan2(tp.tangent.y(), tp.tangent.x()) * 180.0 / 3.14159;
      left_barrier.scale = 1.0;
      objects_.push_back(left_barrier);

      EnvironmentObject right_barrier;
      right_barrier.id = next_id_++;
      right_barrier.type = EnvironmentObjectType::BARRIER;
      right_barrier.position = tp.position - tp.normal * (track_width + 1.0);
      right_barrier.rotation = left_barrier.rotation;
      right_barrier.scale = 1.0;
      objects_.push_back(right_barrier);
    }
  }

  if (config_.place_signs) {
    std::uniform_real_distribution<double> sign_dist(0.0, 1.0);
    for (double d = 0.0; d < track_len; d += 100.0) {
      if (sign_dist(rng_) < 0.3) {
        const auto& tp = track_->at(d);
        EnvironmentObject sign;
        sign.id = next_id_++;
        sign.type = EnvironmentObjectType::SIGN;
        sign.position = tp.position + tp.normal * 12.0;
        sign.rotation = std::atan2(tp.normal.y(), tp.normal.x()) * 180.0 / 3.14159;
        sign.scale = 1.0;
        objects_.push_back(sign);
      }
    }
  }
}

void EnvironmentGenerator::clear() {
  objects_.clear();
  next_id_ = 1;
}

std::vector<EnvironmentObject> EnvironmentGenerator::objects_of_type(EnvironmentObjectType type) const {
  std::vector<EnvironmentObject> result;
  for (const auto& obj : objects_) {
    if (obj.type == type) result.push_back(obj);
  }
  return result;
}

void EnvironmentGenerator::add_object(const EnvironmentObject& obj) {
  objects_.push_back(obj);
}

void EnvironmentGenerator::remove_object(int id) {
  auto it = std::remove_if(objects_.begin(), objects_.end(),
                           [id](const EnvironmentObject& o) { return o.id == id; });
  objects_.erase(it, objects_.end());
}

int EnvironmentGenerator::count_of_type(EnvironmentObjectType type) const {
  return static_cast<int>(std::count_if(objects_.begin(), objects_.end(),
                                        [type](const EnvironmentObject& o) { return o.type == type; }));
}

bool EnvironmentGenerator::is_valid_placement(const Vec2& pos, double min_dist_from_center, double max_dist_from_center) const {
  double dist = distance_to_track_edge(pos);
  return dist >= min_dist_from_center && dist <= max_dist_from_center;
}

double EnvironmentGenerator::distance_to_track_edge(const Vec2& pos) const {
  if (!track_) return 0.0;
  double min_dist = 1e9;
  for (double d = 0.0; d < track_->length(); d += 10.0) {
    const auto& tp = track_->at(d);
    double dist = (pos - tp.position).norm();
    if (dist < min_dist) min_dist = dist;
  }
  return min_dist;
}

Vec2 EnvironmentGenerator::find_placement_position(double min_dist, double max_dist, double current_dist) {
  if (!track_) return Vec2();
  const auto& tp = track_->at(current_dist);
  double offset = (min_dist + max_dist) * 0.5;
  return tp.position + tp.normal * offset;
}

}
