#include <gtest/gtest.h>
#include "vehicle/vehicle.h"
#include "track/track.h"
#include "simulation/simulation.h"
#include "telemetry/telemetry.h"

using namespace p0;

TEST(VehicleState, Initialization) {
  vehicle::VehicleState state;
  EXPECT_DOUBLE_EQ(state.position.x(), 0.0);
  EXPECT_DOUBLE_EQ(state.position.y(), 0.0);
  EXPECT_DOUBLE_EQ(state.speed, 0.0);
  EXPECT_EQ(state.gear, 1);
  EXPECT_EQ(state.lap, 0);
}

TEST(Track, BasicProperties) {
  track::Track track;
  EXPECT_GT(track.length(), 0.0);
  EXPECT_GT(track.at(0.0).width, 0.0);
}

TEST(Simulation, ForwardMovement) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  sim.reset(initial);

  input::InputState input;
  input.throttle = 1.0;

  double prev_distance = 0.0;
  for (int i = 0; i < 1000; ++i) {
    sim.step(input);
    EXPECT_GE(sim.state().speed, 0.0);
    EXPECT_LE(sim.state().speed, 150.0);
  }

  EXPECT_GT(sim.state().distance_along_track, 0.0);
}

TEST(Telemetry, Recording) {
  telemetry::Telemetry tel;
  vehicle::VehicleState state;
  state.position = Vec2(10.0, 20.0);
  state.speed = 60.0;
  state.throttle = 0.5;
  state.steer_angle = 0.1;
  tel.record(state, 1.0 / 120.0);
  ASSERT_EQ(tel.frames().size(), 1u);
  EXPECT_DOUBLE_EQ(tel.frames()[0].speed, 60.0);
  EXPECT_DOUBLE_EQ(tel.frames()[0].position.x(), 10.0);
}

TEST(Steering, ZeroSteerMaintainsStraightLine) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 20.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.3;

  const double start_x = sim.state().position.x();
  const double start_y = sim.state().position.y();
  const double start_heading = sim.state().heading;

  for (int i = 0; i < 600; ++i) {
    sim.step(input);
  }

  EXPECT_NEAR(sim.state().heading, start_heading, 0.01);
  EXPECT_GT(sim.state().position.x(), start_x);
  EXPECT_NEAR(sim.state().position.y(), start_y, 0.5);
}

TEST(Steering, ConstantSteerAtLowSpeed) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 10.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.3;
  input.steering = 0.5;

  const double start_heading = sim.state().heading;
  const double L = sim.state().speed * std::tan(input.steering * 0.5) / 2.5;
  const double expected_heading_change = L * 600.0 * (1.0 / 120.0);

  for (int i = 0; i < 600; ++i) {
    sim.step(input);
  }

  const double actual_heading_change = std::abs(normalize_angle(sim.state().heading - start_heading));
  EXPECT_GT(actual_heading_change, 0.1);
  EXPECT_LT(actual_heading_change, 1.5);
}

TEST(Steering, SpeedAffectsDynamics) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 80.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.3;

  for (int i = 0; i < 600; ++i) {
    sim.step(input);
  }

  const double yaw_rate = sim.state().yaw_rate;
  const double a_y = sim.state().speed * yaw_rate;
  const double a_y_max = sim.state().slip_ratio * kGravity;

  EXPECT_NEAR(yaw_rate, 0.0, 0.5);
}

TEST(Steering, ReturningToZeroSteer) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 30.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.3;
  input.steering = 0.8;

  for (int i = 0; i < 120; ++i) {
    sim.step(input);
  }

  const double heading_after_turn = sim.state().heading;

  input.steering = 0.0;
  for (int i = 0; i < 600; ++i) {
    sim.step(input);
  }

  EXPECT_NEAR(sim.state().yaw_rate, 0.0, 0.01);
}
