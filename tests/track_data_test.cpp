#include <gtest/gtest.h>
#include "track/track_data.h"
#include "track/race_config.h"

using namespace p0::track;

TEST(Transform2D, Defaults) {
  Transform2D t;
  EXPECT_DOUBLE_EQ(t.position.x(), 0.0);
  EXPECT_DOUBLE_EQ(t.position.y(), 0.0);
  EXPECT_DOUBLE_EQ(t.forward.x(), 1.0);
  EXPECT_DOUBLE_EQ(t.forward.y(), 0.0);
}

TEST(MakeTransform, NormalizesForward) {
  p0::Vec2 pos(1.0, 2.0);
  p0::Vec2 fwd(3.0, 4.0);
  Transform2D t = make_transform(pos, fwd);
  EXPECT_DOUBLE_EQ(t.position.x(), 1.0);
  EXPECT_DOUBLE_EQ(t.position.y(), 2.0);
  EXPECT_NEAR(t.forward.norm(), 1.0, 1e-9);
}

TEST(GridDefinition, HasSlot) {
  GridDefinition grid;
  grid.slots.push_back(GridSlot{0, {}, 4.0, 10.0});
  grid.slots.push_back(GridSlot{1, {}, 4.0, 10.0});
  EXPECT_TRUE(grid.has_slot(0));
  EXPECT_TRUE(grid.has_slot(1));
  EXPECT_FALSE(grid.has_slot(2));
}

TEST(GridDefinition, SlotCount) {
  GridDefinition grid;
  EXPECT_EQ(grid.slot_count(), 0);
  grid.slots.push_back(GridSlot{0, {}, 4.0, 10.0});
  EXPECT_EQ(grid.slot_count(), 1);
}

TEST(SpeedDetectionZone, EffectiveLimit) {
  SpeedDetectionZone zone;
  zone.speed_limit_m_s = 16.67;
  zone.tolerance_m_s = 1.39;
  EXPECT_DOUBLE_EQ(zone.effective_limit_m_s(), 18.06);
}

TEST(MergeZone, Length) {
  MergeZone zone;
  zone.start.position = p0::Vec2(0.0, 0.0);
  zone.end.position = p0::Vec2(30.0, 40.0);
  EXPECT_DOUBLE_EQ(zone.length(), 50.0);
}

TEST(PitLaneDefinition, IsValidWithPathAndBoxes) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  EXPECT_TRUE(def.is_valid());
}

TEST(PitLaneDefinition, InvalidWithoutPath) {
  PitLaneDefinition def;
  def.boxes.push_back(PitBox{0, 0, {}, {}, {}, {}, 4.0, 6.0});
  EXPECT_FALSE(def.is_valid());
}

TEST(PitLaneDefinition, InvalidWithoutBoxes) {
  PitLaneDefinition def;
  def.path.push_back(PitLanePathPoint{});
  EXPECT_FALSE(def.is_valid());
}

TEST(TrackData, IsValidWithIdAndLength) {
  TrackData data;
  data.track_id = "monza";
  data.length_m = 5793.0;
  EXPECT_TRUE(data.is_valid());
}

TEST(TrackData, InvalidWithoutId) {
  TrackData data;
  data.length_m = 1000.0;
  EXPECT_FALSE(data.is_valid());
}

TEST(TrackData, InvalidWithZeroLength) {
  TrackData data;
  data.track_id = "monza";
  data.length_m = 0.0;
  EXPECT_FALSE(data.is_valid());
}

TEST(TrackData, MaxGridSlots) {
  TrackData data;
  data.grid.max_slots = 30;
  EXPECT_EQ(data.max_grid_slots(), 30);
}
