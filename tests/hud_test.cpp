#include <gtest/gtest.h>
#include "ui/hud.h"
#include "ui/track_map.h"
#include "track/track.h"

using namespace p0::ui;
using namespace p0::track;
using namespace p0::race;

TEST(Hud, FormatTime) {
  Hud hud;
  EXPECT_EQ(hud.format_time(0.0), "--:--.---");
  EXPECT_EQ(hud.format_time(-1.0), "--:--.---");
  EXPECT_EQ(hud.format_time(65.5), "01:05.500");
  EXPECT_EQ(hud.format_time(125.123), "02:05.123");
}

TEST(Hud, FormatGap) {
  Hud hud;
  EXPECT_EQ(hud.format_gap(0.0), "---");
  EXPECT_EQ(hud.format_gap(-1.0), "---");
  EXPECT_NE(hud.format_gap(1.5), "---");
}

TEST(Hud, UpdateState) {
  Hud hud;
  HudState state;
  state.speedo.speed_kmh = 150.0;
  state.speedo.rpm = 6000.0;
  state.speedo.gear = 3;
  state.position = 2;
  state.total_cars = 10;

  hud.update(state);
  EXPECT_DOUBLE_EQ(hud.state().speedo.speed_kmh, 150.0);
  EXPECT_EQ(hud.state().position, 2);
}

TEST(Hud, LapInfo) {
  Hud hud;
  HudState state;
  state.lap.current_lap = 3;
  state.lap.total_laps = 10;
  state.lap.current_lap_time = 45.5;
  state.lap.best_lap_time = 42.1;

  hud.update(state);
  EXPECT_EQ(hud.state().lap.current_lap, 3);
  EXPECT_EQ(hud.state().lap.total_laps, 10);
}

TEST(TrackMap, DefaultConfig) {
  TrackMap map;
  EXPECT_EQ(map.config().width, 200);
  EXPECT_EQ(map.config().height, 200);
}

TEST(TrackMap, SetTrack) {
  TrackMap map;
  Track track(TrackType::Default);
  map.set_track(&track);
}

TEST(TrackMap, ComputeBounds) {
  TrackMap map;
  Track track(TrackType::Default);
  map.set_track(&track);
  map.compute_bounds();
  EXPECT_TRUE(map.min_x() < map.max_x());
  EXPECT_TRUE(map.min_y() < map.max_y());
}

TEST(TrackMap, WorldToMap) {
  TrackMap map;
  Track track(TrackType::Default);
  map.set_track(&track);
  map.compute_bounds();

  Vec2 center((map.min_x() + map.max_x()) * 0.5f, (map.min_y() + map.max_y()) * 0.5f);
  Vec2 map_pos = map.world_to_map(center);
  EXPECT_GT(map_pos.x(), 0);
  EXPECT_GT(map_pos.y(), 0);
}

TEST(TrackMap, UpdateCars) {
  TrackMap map;
  std::vector<CarPosition> cars;
  CarPosition car1;
  car1.car_id = 1;
  car1.distance_along_track = 100.0;
  car1.is_player = true;
  cars.push_back(car1);

  CarPosition car2;
  car2.car_id = 2;
  car2.distance_along_track = 200.0;
  car2.is_player = false;
  cars.push_back(car2);

  map.update(cars);
  EXPECT_EQ(map.cars().size(), 2u);
}

TEST(TrackMap, TrackToMap) {
  TrackMap map;
  Track track(TrackType::Default);
  map.set_track(&track);
  map.compute_bounds();

  Vec2 map_pos = map.track_to_map(0.0);
  EXPECT_GE(map_pos.x(), 0);
  EXPECT_GE(map_pos.y(), 0);
}
