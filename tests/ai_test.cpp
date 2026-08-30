#include "ai/racing_line.h"
#include "ai/ai_driver.h"
#include "ai/opponent.h"
#include "track/track.h"
#include "vehicle/vehicle.h"
#include "input/input.h"

#include <gtest/gtest.h>
#include <cmath>
#include <algorithm>

using namespace p0;
using namespace p0::ai;

TEST(RacingLine, GeneratesPoints) {
  track::Track track;
  RacingLineOptimizer optimizer(track);

  EXPECT_FALSE(optimizer.points().empty());
  EXPECT_EQ(optimizer.points().size(), track.sample_count());
}

TEST(RacingLine, PointsHaveValidPositions) {
  track::Track track;
  RacingLineOptimizer optimizer(track);

  for (const auto& rp : optimizer.points()) {
    EXPECT_FALSE(std::isnan(rp.position.x()));
    EXPECT_FALSE(std::isnan(rp.position.y()));
    EXPECT_GT(rp.speed_m_s, 0.0);
    EXPECT_LE(rp.speed_m_s, 150.0);
  }
}

TEST(RacingLine, ConvertToSamples) {
  track::Track track;
  RacingLineOptimizer optimizer(track);

  auto samples = optimizer.to_racing_line_samples();
  EXPECT_EQ(samples.size(), optimizer.points().size());

  for (const auto& s : samples) {
    EXPECT_FALSE(std::isnan(s.transform.position.x()));
    EXPECT_FALSE(std::isnan(s.transform.position.y()));
    EXPECT_GT(s.speed_m_s, 0.0);
  }
}

TEST(RacingLine, TargetSpeedAtDistance) {
  track::Track track;
  RacingLineOptimizer optimizer(track);

  double speed = optimizer.target_speed_at(0.0);
  EXPECT_GT(speed, 0.0);
  EXPECT_LE(speed, 150.0);
}

TEST(RacingLine, StraightHasLowOffset) {
  track::Track track;
  RacingLineOptimizer optimizer(track);

  double max_offset = 0.0;
  for (const auto& rp : optimizer.points()) {
    max_offset = std::max(max_offset, std::abs(rp.lateral_offset));
  }

  EXPECT_GT(max_offset, 0.0);
}

TEST(AIDriver, AcceptsRacingLine) {
  track::Track track;
  AIDriver driver;
  driver.set_track(track);

  auto samples = RacingLineOptimizer(track).to_racing_line_samples();
  driver.set_racing_line(samples);

  EXPECT_FALSE(driver.last_input().throttle);
}

TEST(AIDriver, RacingLineProducesSteering) {
  track::Track track;
  AIDriver driver;
  driver.set_track(track);
  driver.set_racing_line(RacingLineOptimizer(track).to_racing_line_samples());

  vehicle::VehicleState state;
  state.position = track.get_start_position();
  state.heading = track.get_start_heading();
  state.speed = 50.0;
  state.rpm = 3000.0;
  state.gear = 2;
  state.distance_along_track = 0.0;

  driver.update(state, 0.016);

  EXPECT_TRUE(std::isfinite(driver.last_input().steering));
  EXPECT_GE(driver.last_input().steering, -1.0);
  EXPECT_LE(driver.last_input().steering, 1.0);
}

TEST(AIDriver, RacingLineProducesThrottleBrake) {
  track::Track track;
  AIDriver driver;
  driver.set_track(track);
  driver.set_racing_line(RacingLineOptimizer(track).to_racing_line_samples());

  vehicle::VehicleState state;
  state.position = track.get_start_position();
  state.heading = track.get_start_heading();
  state.speed = 0.0;
  state.rpm = 1000.0;
  state.gear = 1;
  state.distance_along_track = 0.0;

  driver.update(state, 0.016);

  EXPECT_GE(driver.last_input().throttle, 0.0);
  EXPECT_LE(driver.last_input().throttle, 1.0);
  EXPECT_GE(driver.last_input().brake, 0.0);
  EXPECT_LE(driver.last_input().brake, 1.0);
}

TEST(AIDriver, DifficultyPresets) {
  AIDriverParams easy;
  easy.look_ahead_distance = 25.0;
  easy.steering_gain = 0.7;

  AIDriverParams medium;
  medium.look_ahead_distance = 35.0;
  medium.steering_gain = 0.9;

  AIDriverParams hard;
  hard.look_ahead_distance = 45.0;
  hard.steering_gain = 1.0;

  EXPECT_LT(easy.look_ahead_distance, medium.look_ahead_distance);
  EXPECT_LT(medium.look_ahead_distance, hard.look_ahead_distance);
}

TEST(Opponent, Creation) {
  Opponent opp(AIDriverParams{}, "TestAI");
  EXPECT_EQ(opp.state().name, "TestAI");
  EXPECT_EQ(opp.state().car_id, -1);
}

TEST(Opponent, Update) {
  track::Track track;
  Opponent opp;
  opp.set_track(track);
  opp.set_racing_line(RacingLineOptimizer(track).to_racing_line_samples());

  vehicle::VehicleState state;
  state.position = track.get_start_position();
  state.heading = track.get_start_heading();
  state.speed = 50.0;
  state.rpm = 3000.0;
  state.gear = 2;
  state.distance_along_track = 0.0;

  auto input = opp.update(state, 0.016);
  EXPECT_TRUE(std::isfinite(input.steering));
}

TEST(OpponentManager, AddOpponent) {
  OpponentManager manager;
  int id = manager.add_opponent(AIDriverParams{}, "AI1");
  EXPECT_GT(id, 0);
  EXPECT_EQ(manager.count(), 1);
}

TEST(OpponentManager, RemoveOpponent) {
  OpponentManager manager;
  int id = manager.add_opponent(AIDriverParams{}, "AI1");
  manager.remove_opponent(id);
  EXPECT_EQ(manager.count(), 0);
}

TEST(OpponentManager, UpdateAll) {
  track::Track track;
  OpponentManager manager(&track);
  manager.set_racing_line(RacingLineOptimizer(track).to_racing_line_samples());

  manager.add_opponent(AIDriverParams{}, "AI1");
  manager.add_opponent(AIDriverParams{}, "AI2");

  std::vector<vehicle::VehicleState> states(2);
  for (auto& s : states) {
    s.position = track.get_start_position();
    s.heading = track.get_start_heading();
    s.speed = 50.0;
    s.rpm = 3000.0;
    s.gear = 2;
    s.distance_along_track = 0.0;
  }

  auto inputs = manager.update_all(states, 0.016);
  EXPECT_EQ(inputs.size(), 2u);
}

TEST(OpponentManager, GetOpponent) {
  OpponentManager manager;
  int id = manager.add_opponent(AIDriverParams{}, "AI1");

  const Opponent* opp = manager.get(id);
  EXPECT_NE(opp, nullptr);
  EXPECT_EQ(opp->state().name, "AI1");

  EXPECT_EQ(manager.get(9999), nullptr);
}

TEST(AIDriver, SetNearbyCars) {
  track::Track track;
  AIDriver driver;
  driver.set_track(track);
  driver.set_racing_line(RacingLineOptimizer(track).to_racing_line_samples());

  vehicle::VehicleState nearby;
  nearby.position = track.get_start_position() + Vec2(5.0, 0.0);
  nearby.heading = 0.0;
  nearby.speed = 50.0;

  driver.set_nearby_cars({nearby});

  vehicle::VehicleState state;
  state.position = track.get_start_position();
  state.heading = track.get_start_heading();
  state.speed = 50.0;
  state.rpm = 3000.0;
  state.gear = 2;
  state.distance_along_track = 0.0;

  driver.update(state, 0.016);
  EXPECT_TRUE(std::isfinite(driver.last_input().steering));
}

TEST(AIDriver, OvertakingAdjustsTarget) {
  track::Track track;
  AIDriverParams params;
  params.overtake_aggression = 1.0;
  params.enable_defense = true;
  AIDriver driver(params);
  driver.set_track(track);
  driver.set_racing_line(RacingLineOptimizer(track).to_racing_line_samples());

  vehicle::VehicleState nearby;
  nearby.position = track.get_start_position() + Vec2(3.0, 0.0);
  nearby.heading = 0.0;
  nearby.speed = 50.0;
  driver.set_nearby_cars({nearby});

  vehicle::VehicleState state;
  state.position = track.get_start_position();
  state.heading = track.get_start_heading();
  state.speed = 50.0;
  state.rpm = 3000.0;
  state.gear = 2;
  state.distance_along_track = 0.0;

  driver.update(state, 0.016);
  EXPECT_TRUE(std::isfinite(driver.last_input().steering));
}

TEST(AIDriver, DefenseAdjustsTarget) {
  track::Track track;
  AIDriverParams params;
  params.defense_willingness = 1.0;
  params.enable_defense = true;
  AIDriver driver(params);
  driver.set_track(track);
  driver.set_racing_line(RacingLineOptimizer(track).to_racing_line_samples());

  vehicle::VehicleState ahead;
  ahead.position = track.get_start_position() - Vec2(3.0, 0.0);
  ahead.heading = 0.0;
  ahead.speed = 50.0;
  driver.set_nearby_cars({ahead});

  vehicle::VehicleState state;
  state.position = track.get_start_position();
  state.heading = track.get_start_heading();
  state.speed = 50.0;
  state.rpm = 3000.0;
  state.gear = 2;
  state.distance_along_track = 0.0;

  driver.update(state, 0.016);
  EXPECT_TRUE(std::isfinite(driver.last_input().steering));
}
