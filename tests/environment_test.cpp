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
