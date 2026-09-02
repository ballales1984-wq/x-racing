#pragma once

#include "common.h"
#include "track/track.h"
#include <vector>
#include <random>
#include <string>

namespace p0::environment {

enum class EnvironmentObjectType : uint8_t {
  TREE,
  ROCK,
  GRASS,
  BARRIER,
  SIGN
};

struct EnvironmentObject {
  int id = -1;
  EnvironmentObjectType type = EnvironmentObjectType::TREE;
  Vec2 position;
  double rotation = 0.0;
  double scale = 1.0;
  int variant = 0;
};

struct EnvironmentZone {
  double min_distance_from_center = 10.0;
  double max_distance_from_center = 50.0;
  double density = 0.3;
  int max_objects = 100;
};

struct EnvironmentConfig {
  EnvironmentZone tree_zone;
  EnvironmentZone rock_zone;
  EnvironmentZone grass_zone;
  int seed = 42;
  bool place_barriers = true;
  bool place_signs = true;

  EnvironmentConfig() {
    tree_zone.min_distance_from_center = 15.0;
    tree_zone.max_distance_from_center = 60.0;
    tree_zone.density = 0.2;
    tree_zone.max_objects = 200;

    rock_zone.min_distance_from_center = 12.0;
    rock_zone.max_distance_from_center = 40.0;
    rock_zone.density = 0.1;
    rock_zone.max_objects = 50;

    grass_zone.min_distance_from_center = 8.0;
    grass_zone.max_distance_from_center = 30.0;
    grass_zone.density = 0.5;
    grass_zone.max_objects = 500;
  }
};

class EnvironmentGenerator {
 public:
  EnvironmentGenerator();

  void set_track(const track::Track* track);
  void set_config(const EnvironmentConfig& config);
  const EnvironmentConfig& config() const { return config_; }

  void generate();
  void clear();

  const std::vector<EnvironmentObject>& objects() const { return objects_; }
  std::vector<EnvironmentObject> objects_of_type(EnvironmentObjectType type) const;

  void add_object(const EnvironmentObject& obj);
  void remove_object(int id);

  int count() const { return static_cast<int>(objects_.size()); }
  int count_of_type(EnvironmentObjectType type) const;

 private:
  void generate_zone_objects();
  void generate_dynamic_barriers();
  void generate_braking_signs();

  bool is_valid_placement(const Vec2& pos, double min_dist_from_center, double max_dist_from_center) const;
  double distance_to_track_edge(const Vec2& pos) const;
  Vec2 find_placement_position(double min_dist, double max_dist, double current_dist);

  const track::Track* track_ = nullptr;
  EnvironmentConfig config_;
  std::vector<EnvironmentObject> objects_;
  std::mt19937 rng_;
  int next_id_ = 1;
};

}
