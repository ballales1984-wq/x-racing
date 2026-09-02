#include "environment/environment_generator.h"
#include <algorithm>
#include <cmath>

namespace p0::environment {

//! @brief Constructs the environment generator with a random seed.
EnvironmentGenerator::EnvironmentGenerator() {
  std::random_device rd;
  rng_.seed(rd());
}

//! @brief Sets the track reference for object placement.
//! @param track Pointer to the track data.
void EnvironmentGenerator::set_track(const track::Track* track) {
  track_ = track;
}

//! @brief Sets the generation configuration and reseeds the RNG.
//! @param config The environment configuration.
void EnvironmentGenerator::set_config(const EnvironmentConfig& config) {
  config_ = config;
  rng_.seed(config.seed);
}

//! @brief Generates trackside objects based on the configuration.
//!        Places trees, rocks, grass, barriers, and signs along the track.
void EnvironmentGenerator::generate() {
  clear();
  if (!track_) return;

  double track_len = track_->length();
  if (track_len <= 0.0) return;

  generate_zone_objects();

  if (config_.place_barriers) {
    generate_dynamic_barriers();
  }

  if (config_.place_signs) {
    generate_braking_signs();
  }
}

//! @brief Generates environment zone objects (trees, rocks, grass) using zone configs.
void EnvironmentGenerator::generate_zone_objects() {
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
    double track_half_w = tp.width * 0.5;

    // Tree placement
    if (prob_dist(rng_) < config_.tree_zone.density &&
        count_of_type(EnvironmentObjectType::TREE) < config_.tree_zone.max_objects) {
      double min_d = std::max(track_half_w + 3.0, config_.tree_zone.min_distance_from_center);
      double max_d = std::max(min_d + 5.0, config_.tree_zone.max_distance_from_center);
      Vec2 pos = find_placement_position(min_d, max_d, d);
      if (is_valid_placement(pos, min_d - 2.0, max_d + 15.0)) {
        EnvironmentObject obj;
        obj.id = next_id_++;
        obj.type = EnvironmentObjectType::TREE;
        obj.position = pos;
        obj.rotation = rot_dist(rng_);
        obj.scale = scale_dist(rng_);
        obj.variant = variant_dist(rng_);
        objects_.push_back(obj);
      }
    }

    // Rock placement
    if (prob_dist(rng_) < config_.rock_zone.density &&
        count_of_type(EnvironmentObjectType::ROCK) < config_.rock_zone.max_objects) {
      double min_d = std::max(track_half_w + 2.0, config_.rock_zone.min_distance_from_center);
      double max_d = std::max(min_d + 3.0, config_.rock_zone.max_distance_from_center);
      Vec2 pos = find_placement_position(min_d, max_d, d);
      if (is_valid_placement(pos, min_d - 2.0, max_d + 10.0)) {
        EnvironmentObject obj;
        obj.id = next_id_++;
        obj.type = EnvironmentObjectType::ROCK;
        obj.position = pos;
        obj.rotation = rot_dist(rng_);
        obj.scale = scale_dist(rng_) * 0.5;
        obj.variant = variant_dist(rng_) % 3;
        objects_.push_back(obj);
      }
    }

    // Grass placement
    if (prob_dist(rng_) < config_.grass_zone.density &&
        count_of_type(EnvironmentObjectType::GRASS) < config_.grass_zone.max_objects) {
      double min_d = std::max(track_half_w + 1.0, config_.grass_zone.min_distance_from_center);
      double max_d = std::max(min_d + 2.0, config_.grass_zone.max_distance_from_center);
      Vec2 pos = find_placement_position(min_d, max_d, d);
      if (is_valid_placement(pos, min_d - 1.5, max_d + 8.0)) {
        EnvironmentObject obj;
        obj.id = next_id_++;
        obj.type = EnvironmentObjectType::GRASS;
        obj.position = pos;
        obj.rotation = rot_dist(rng_);
        obj.scale = scale_dist(rng_) * 0.3;
        obj.variant = variant_dist(rng_) % 2;
        objects_.push_back(obj);
      }
    }
  }
}

//! @brief Generates dynamic barriers along straight sections and outer corners.
void EnvironmentGenerator::generate_dynamic_barriers() {
  if (!track_) return;
  double track_len = track_->length();
  constexpr double barrier_step = 8.0;

  for (double d = 0.0; d < track_len; d += barrier_step) {
    const auto& tp = track_->at(d);
    double track_half_w = tp.width * 0.5;
    double abs_curv = std::abs(tp.curvature);
    double rad_heading = std::atan2(tp.tangent.y(), tp.tangent.x()) * 180.0 / 3.14159265358979323846;

    bool is_corner = (abs_curv > 0.008);
    bool turn_left = (tp.curvature > 0.0);

    EnvironmentObject left_barrier;
    left_barrier.id = next_id_++;
    left_barrier.type = EnvironmentObjectType::BARRIER;
    left_barrier.position = tp.position + tp.normal * (track_half_w + 1.0);
    left_barrier.rotation = rad_heading;
    left_barrier.scale = 1.0;
    left_barrier.variant = (is_corner && !turn_left) ? 1 : 0;
    objects_.push_back(left_barrier);

    EnvironmentObject right_barrier;
    right_barrier.id = next_id_++;
    right_barrier.type = EnvironmentObjectType::BARRIER;
    right_barrier.position = tp.position - tp.normal * (track_half_w + 1.0);
    right_barrier.rotation = rad_heading;
    right_barrier.scale = 1.0;
    right_barrier.variant = (is_corner && turn_left) ? 1 : 0;
    objects_.push_back(right_barrier);
  }
}

//! @brief Generates braking distance marker signs before corners.
void EnvironmentGenerator::generate_braking_signs() {
  if (!track_) return;
  double track_len = track_->length();
  if (track_len <= 0.0) return;

  std::vector<double> corner_entry_distances;
  constexpr double scan_step = 10.0;

  for (double d = 0.0; d < track_len; d += scan_step) {
    const auto& curr = track_->at(d);
    double next_d = std::fmod(d + scan_step, track_len);
    const auto& next_pt = track_->at(next_d);

    if (std::abs(curr.curvature) < 0.005 && std::abs(next_pt.curvature) >= 0.007) {
      corner_entry_distances.push_back(next_d);
    }
  }

  if (corner_entry_distances.empty()) {
    for (double d = 0.0; d < track_len; d += scan_step) {
      if (std::abs(track_->at(d).curvature) >= 0.007) {
        corner_entry_distances.push_back(d);
        break;
      }
    }
  }

  const int sign_distances[3] = {150, 100, 50};

  for (double turn_d : corner_entry_distances) {
    const auto& turn_tp = track_->at(turn_d);
    Vec2 side_dir = (turn_tp.curvature > 0.0) ? -turn_tp.normal : turn_tp.normal;

    for (int dist_m : sign_distances) {
      double sign_d = turn_d - static_cast<double>(dist_m);
      while (sign_d < 0.0) sign_d += track_len;

      const auto& tp = track_->at(sign_d);
      double track_half_w = tp.width * 0.5;

      EnvironmentObject sign;
      sign.id = next_id_++;
      sign.type = EnvironmentObjectType::SIGN;
      sign.position = tp.position + side_dir * (track_half_w + 2.5);
      sign.rotation = std::atan2(tp.normal.y(), tp.normal.x()) * 180.0 / 3.14159265358979323846;
      sign.scale = 1.0;
      sign.variant = dist_m;
      objects_.push_back(sign);
    }
  }

  if (objects_of_type(EnvironmentObjectType::SIGN).empty()) {
    std::uniform_real_distribution<double> sign_dist(0.0, 1.0);
    for (double d = 0.0; d < track_len; d += 100.0) {
      if (sign_dist(rng_) < 0.3) {
        const auto& tp = track_->at(d);
        EnvironmentObject sign;
        sign.id = next_id_++;
        sign.type = EnvironmentObjectType::SIGN;
        sign.position = tp.position + tp.normal * (tp.width * 0.5 + 3.0);
        sign.rotation = std::atan2(tp.normal.y(), tp.normal.x()) * 180.0 / 3.14159265358979323846;
        sign.scale = 1.0;
        sign.variant = 0;
        objects_.push_back(sign);
      }
    }
  }
}

//! @brief Clears all generated objects and resets the ID counter.
void EnvironmentGenerator::clear() {
  objects_.clear();
  next_id_ = 1;
}

//! @brief Returns all objects of a specific type.
//! @param type The object type to filter.
//! @return Vector of EnvironmentObject matching the type.
std::vector<EnvironmentObject> EnvironmentGenerator::objects_of_type(EnvironmentObjectType type) const {
  std::vector<EnvironmentObject> result;
  for (const auto& obj : objects_) {
    if (obj.type == type) result.push_back(obj);
  }
  return result;
}

//! @brief Adds a custom object to the environment.
//! @param obj The object to add.
void EnvironmentGenerator::add_object(const EnvironmentObject& obj) {
  objects_.push_back(obj);
}

//! @brief Removes an object by its ID.
//! @param id The ID of the object to remove.
void EnvironmentGenerator::remove_object(int id) {
  auto it = std::remove_if(objects_.begin(), objects_.end(),
                           [id](const EnvironmentObject& o) { return o.id == id; });
  objects_.erase(it, objects_.end());
}

//! @brief Counts objects of a specific type.
//! @param type The object type to count.
//! @return Number of objects of the given type.
int EnvironmentGenerator::count_of_type(EnvironmentObjectType type) const {
  return static_cast<int>(std::count_if(objects_.begin(), objects_.end(),
                                        [type](const EnvironmentObject& o) { return o.type == type; }));
}

//! @brief Checks if a position is within valid placement bounds.
//! @param pos The position to check.
//! @param min_dist_from_center Minimum distance from track centerline.
//! @param max_dist_from_center Maximum distance from track centerline.
//! @return true if the position is within the valid zone.
bool EnvironmentGenerator::is_valid_placement(const Vec2& pos, double min_dist_from_center, double max_dist_from_center) const {
  double dist = distance_to_track_edge(pos);
  return dist >= min_dist_from_center && dist <= max_dist_from_center;
}

//! @brief Calculates the minimum distance from a position to the track centerline.
//! @param pos The position to measure.
//! @return Minimum distance to track centerline.
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

//! @brief Finds a placement position at a given distance along the track.
//! @param min_dist Minimum distance from centerline.
//! @param max_dist Maximum distance from centerline.
//! @param current_dist Distance along the track centerline.
//! @return Position vector for object placement.
Vec2 EnvironmentGenerator::find_placement_position(double min_dist, double max_dist, double current_dist) {
  if (!track_) return Vec2();
  const auto& tp = track_->at(current_dist);
  std::uniform_real_distribution<double> dist_samp(min_dist, max_dist);
  std::uniform_real_distribution<double> side_samp(0.0, 1.0);
  double offset = dist_samp(rng_);
  Vec2 dir = (side_samp(rng_) < 0.5) ? tp.normal : -tp.normal;
  return tp.position + dir * offset;
}

}