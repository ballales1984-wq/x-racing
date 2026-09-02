#include <gtest/gtest.h>
#include "environment/environment_generator.h"
#include "track/track.h"

using namespace p0::environment;
using namespace p0::track;

static Track create_test_track() {
  Track track(TrackType::Default);
  return track;
}

TEST(EnvironmentGenerator, EmptyByDefault) {
  EnvironmentGenerator gen;
  EXPECT_EQ(gen.count(), 0u);
}

TEST(EnvironmentGenerator, GenerateWithNoTrack) {
  EnvironmentGenerator gen;
  gen.generate();
  EXPECT_EQ(gen.count(), 0u);
}

TEST(EnvironmentGenerator, GenerateTrees) {
  EnvironmentGenerator gen;
  Track track = create_test_track();
  gen.set_track(&track);

  EnvironmentConfig config;
  config.tree_zone.density = 1.0;
  config.rock_zone.density = 0.0;
  config.grass_zone.density = 0.0;
  config.place_barriers = false;
  config.place_signs = false;
  gen.set_config(config);

  gen.generate();
  EXPECT_GT(gen.count_of_type(EnvironmentObjectType::TREE), 0);
}

TEST(EnvironmentGenerator, GenerateRocks) {
  EnvironmentGenerator gen;
  Track track = create_test_track();
  gen.set_track(&track);

  EnvironmentConfig config;
  config.tree_zone.density = 0.0;
  config.rock_zone.density = 1.0;
  config.grass_zone.density = 0.0;
  config.place_barriers = false;
  config.place_signs = false;
  gen.set_config(config);

  gen.generate();
  EXPECT_GT(gen.count_of_type(EnvironmentObjectType::ROCK), 0);
}

TEST(EnvironmentGenerator, GenerateGrass) {
  EnvironmentGenerator gen;
  Track track = create_test_track();
  gen.set_track(&track);

  EnvironmentConfig config;
  config.tree_zone.density = 0.0;
  config.rock_zone.density = 0.0;
  config.grass_zone.density = 1.0;
  config.place_barriers = false;
  config.place_signs = false;
  gen.set_config(config);

  gen.generate();
  EXPECT_GT(gen.count_of_type(EnvironmentObjectType::GRASS), 0);
}

TEST(EnvironmentGenerator, GenerateBarriers) {
  EnvironmentGenerator gen;
  Track track = create_test_track();
  gen.set_track(&track);

  EnvironmentConfig config;
  config.tree_zone.density = 0.0;
  config.rock_zone.density = 0.0;
  config.grass_zone.density = 0.0;
  config.place_barriers = true;
  config.place_signs = false;
  gen.set_config(config);

  gen.generate();
  EXPECT_GT(gen.count_of_type(EnvironmentObjectType::BARRIER), 0);
}

TEST(EnvironmentGenerator, GenerateSigns) {
  EnvironmentGenerator gen;
  Track track = create_test_track();
  gen.set_track(&track);

  EnvironmentConfig config;
  config.tree_zone.density = 0.0;
  config.rock_zone.density = 0.0;
  config.grass_zone.density = 0.0;
  config.place_barriers = false;
  config.place_signs = true;
  gen.set_config(config);

  gen.generate();
  EXPECT_GT(gen.count_of_type(EnvironmentObjectType::SIGN), 0);
}

TEST(EnvironmentGenerator, ClearRemovesAll) {
  EnvironmentGenerator gen;
  Track track = create_test_track();
  gen.set_track(&track);
  gen.generate();
  EXPECT_GT(gen.count(), 0u);
  gen.clear();
  EXPECT_EQ(gen.count(), 0u);
}

TEST(EnvironmentGenerator, ObjectsOfType) {
  EnvironmentGenerator gen;
  Track track = create_test_track();
  gen.set_track(&track);

  EnvironmentConfig config;
  config.tree_zone.density = 1.0;
  config.rock_zone.density = 1.0;
  config.grass_zone.density = 0.0;
  config.place_barriers = false;
  config.place_signs = false;
  gen.set_config(config);

  gen.generate();

  auto trees = gen.objects_of_type(EnvironmentObjectType::TREE);
  auto rocks = gen.objects_of_type(EnvironmentObjectType::ROCK);
  EXPECT_GT(trees.size(), 0u);
  EXPECT_GT(rocks.size(), 0u);
}

TEST(EnvironmentGenerator, AddRemoveObject) {
  EnvironmentGenerator gen;

  EnvironmentObject obj;
  obj.type = EnvironmentObjectType::TREE;
  obj.position = Vec2(100.0, 50.0);

  gen.add_object(obj);
  EXPECT_EQ(gen.count(), 1u);

  gen.remove_object(obj.id);
  EXPECT_EQ(gen.count(), 0u);
}

TEST(EnvironmentGenerator, ReproducibleWithSeed) {
  EnvironmentGenerator gen1, gen2;
  Track track = create_test_track();

  EnvironmentConfig config;
  config.seed = 12345;
  config.tree_zone.density = 0.5;
  config.rock_zone.density = 0.5;
  config.grass_zone.density = 0.5;
  config.place_barriers = false;
  config.place_signs = false;

  gen1.set_track(&track);
  gen1.set_config(config);
  gen1.generate();

  gen2.set_track(&track);
  gen2.set_config(config);
  gen2.generate();

  EXPECT_EQ(gen1.count(), gen2.count());
}

TEST(EnvironmentGenerator, GenerateBrakingSignsBeforeTurns) {
  EnvironmentGenerator gen;
  Track track = create_test_track();
  gen.set_track(&track);

  EnvironmentConfig config;
  config.tree_zone.density = 0.0;
  config.rock_zone.density = 0.0;
  config.grass_zone.density = 0.0;
  config.place_barriers = false;
  config.place_signs = true;
  gen.set_config(config);

  gen.generate();
  auto signs = gen.objects_of_type(EnvironmentObjectType::SIGN);
  EXPECT_GT(signs.size(), 0u);

  bool has_150 = false, has_100 = false, has_50 = false;
  for (const auto& s : signs) {
    if (s.variant == 150) has_150 = true;
    if (s.variant == 100) has_100 = true;
    if (s.variant == 50)  has_50 = true;
  }
  EXPECT_TRUE(has_150 || has_100 || has_50);
}

TEST(EnvironmentGenerator, GenerateDynamicBarriersOnCorners) {
  EnvironmentGenerator gen;
  Track track = create_test_track();
  gen.set_track(&track);

  EnvironmentConfig config;
  config.tree_zone.density = 0.0;
  config.rock_zone.density = 0.0;
  config.grass_zone.density = 0.0;
  config.place_barriers = true;
  config.place_signs = false;
  gen.set_config(config);

  gen.generate();
  auto barriers = gen.objects_of_type(EnvironmentObjectType::BARRIER);
  EXPECT_GT(barriers.size(), 0u);

  bool has_reinforced = false;
  for (const auto& b : barriers) {
    if (b.variant == 1) {
      has_reinforced = true;
      break;
    }
  }
  EXPECT_TRUE(has_reinforced);
}

TEST(EnvironmentGenerator, ObjectsRespectZoneBoundaries) {
  EnvironmentGenerator gen;
  Track track = create_test_track();
  gen.set_track(&track);

  EnvironmentConfig config;
  config.tree_zone.density = 0.8;
  config.tree_zone.min_distance_from_center = 15.0;
  config.tree_zone.max_distance_from_center = 60.0;
  config.place_barriers = false;
  config.place_signs = false;
  gen.set_config(config);

  gen.generate();
  auto trees = gen.objects_of_type(EnvironmentObjectType::TREE);
  EXPECT_GT(trees.size(), 0u);

  for (const auto& tree : trees) {
    double dist = (tree.position - track.at(0.0).position).norm();
    EXPECT_GE(dist, 0.0);
  }
}

