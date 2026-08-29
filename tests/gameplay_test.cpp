// Project 0 — unit tests for gameplay loop and lap timing
#include <gtest/gtest.h>
#include "game/gameplay.h"
#include "simulation/simulation.h"
#include "telemetry/telemetry.h"
#include "track/track.h"
#include "input/input.h"
#include <memory>
#include <windows.h>

using namespace p0;

// Fake input manager that returns fixed input
class FakeInputManager : public input::InputManager {
 public:
  explicit FakeInputManager(input::InputState fixed_input) : fixed_(fixed_input) {}

  input::InputState poll() override {
    return fixed_;
  }

  bool is_key_down(int key_code) override {
    if (key_code == VK_ESCAPE) return false;
    return false;
  }

 private:
  input::InputState fixed_;
};

// Gameplay should initialize with correct defaults
TEST(Gameplay, Initialization) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<FakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));
  const auto& state = gp.state();
  EXPECT_TRUE(state.running);
  EXPECT_EQ(state.current_lap, 0);
  EXPECT_DOUBLE_EQ(state.best_lap_time, 0.0);
  EXPECT_DOUBLE_EQ(state.current_lap_time, 0.0);
}

// Gameplay update_lap_timing should advance current lap time
TEST(Gameplay, LapTimingAdvances) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<FakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  simulation::SimulationResult result;
  result.time = 1.0 / 60.0;
  result.state.distance_along_track = 10.0;
  gp.update_lap_timing(result);
  EXPECT_DOUBLE_EQ(gp.state().current_lap_time, 1.0 / 60.0);
}

// Gameplay should record a lap when lap advances past 0
TEST(Gameplay, RecordsLapWhenAdvancing) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<FakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Accumulate some time during lap 0
  simulation::SimulationResult result;
  result.time = 10.0;
  result.state.lap = 0;
  gp.update_lap_timing(result);

  // Simulate first lap completion (0 -> 1) with negligible transition dt
  result.time = 10.001;
  result.state.lap = 1;
  result.state.distance_along_track = track.length() + 10.0;
  gp.update_lap_timing(result);

  // First lap completion is recorded (current_lap transitions 0 -> 1)
  EXPECT_EQ(gp.state().lap_times.size(), 1u);
  EXPECT_DOUBLE_EQ(gp.state().lap_times[0].lap_time, 10.0);

  // Accumulate time during lap 1
  result.time = 15.0;
  result.state.lap = 1;
  gp.update_lap_timing(result);

  // Simulate second lap completion (1 -> 2) with negligible transition dt
  result.time = 15.001;
  result.state.lap = 2;
  result.state.distance_along_track = track.length() * 2 + 10.0;
  gp.update_lap_timing(result);

  EXPECT_EQ(gp.state().lap_times.size(), 2u);
  EXPECT_DOUBLE_EQ(gp.state().lap_times[1].lap_time, 5.0);
}

// Gameplay should mark lap invalid when off track warning is present
TEST(Gameplay, OffTrackInvalidatesLap) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<FakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Accumulate some time during lap 0
  simulation::SimulationResult result;
  result.time = 10.0;
  result.state.lap = 0;
  gp.update_lap_timing(result);

  // Simulate first lap completion with negligible transition dt
  result.time = 10.001;
  result.state.lap = 1;
  result.state.distance_along_track = track.length() + 10.0;
  gp.update_lap_timing(result);

  // First lap is recorded (current_lap transitions 0 -> 1)
  EXPECT_EQ(gp.state().lap_times.size(), 1u);

  // Now simulate second lap with off_track warning already set
  gp.set_off_track_warning(true);
  result.time = 15.001;
  result.state.lap = 2;
  result.state.distance_along_track = track.length() * 2 + 10.0;
  gp.update_lap_timing(result);

  EXPECT_EQ(gp.state().lap_times.size(), 2u);
  EXPECT_FALSE(gp.state().lap_times[1].valid);
}

// Gameplay should track best lap time across multiple laps
TEST(Gameplay, BestLapTimeTracking) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<FakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Accumulate 50s during lap 0
  simulation::SimulationResult result;
  result.time = 50.0;
  result.state.lap = 0;
  gp.update_lap_timing(result);

  // First lap completion: transition dt is negligible
  result.time = 50.001;
  result.state.lap = 1;
  result.state.distance_along_track = track.length() + 10.0;
  gp.update_lap_timing(result);

  EXPECT_EQ(gp.state().lap_times.size(), 1u);
  EXPECT_DOUBLE_EQ(gp.state().lap_times[0].lap_time, 50.0);

  // Accumulate 40s during lap 1
  result.time = 90.001;
  result.state.lap = 1;
  gp.update_lap_timing(result);

  // Second lap completion: lap time ≈ 40s
  result.time = 90.002;
  result.state.lap = 2;
  result.state.distance_along_track = track.length() * 2 + 10.0;
  gp.update_lap_timing(result);

  EXPECT_EQ(gp.state().lap_times.size(), 2u);
  EXPECT_NEAR(gp.state().best_lap_time, 40.0, 0.01);

  // Accumulate 45s during lap 2
  result.time = 135.002;
  result.state.lap = 2;
  gp.update_lap_timing(result);

  // Third lap completion: lap time ≈ 45s (not better than 40s)
  result.time = 135.003;
  result.state.lap = 3;
  result.state.distance_along_track = track.length() * 3 + 10.0;
  gp.update_lap_timing(result);

  EXPECT_EQ(gp.state().lap_times.size(), 3u);
  EXPECT_NEAR(gp.state().best_lap_time, 40.0, 0.01);
}

// Gameplay should not invalidate lap for off-track on lap 0
TEST(Gameplay, OffTrackDoesNotRecordLap0) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<FakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  simulation::SimulationResult result;
  result.time = 5.0;
  result.off_track = true;
  result.state.lap = 0;
  result.state.distance_along_track = 10.0;
  gp.update_lap_timing(result);

  EXPECT_TRUE(gp.state().lap_times.empty());
}

// InputState should initialize with zero values
TEST(InputState, Defaults) {
  input::InputState input;
  EXPECT_DOUBLE_EQ(input.throttle, 0.0);
  EXPECT_DOUBLE_EQ(input.brake, 0.0);
  EXPECT_DOUBLE_EQ(input.steering, 0.0);
  EXPECT_FALSE(input.upshift);
  EXPECT_FALSE(input.downshift);
  EXPECT_FALSE(input.reset);
  EXPECT_FALSE(input.enter_exit_box);
}

// InputState should allow explicit values
TEST(InputState, ExplicitValues) {
  input::InputState input;
  input.throttle = 1.0;
  input.brake = 0.5;
  input.steering = -0.8;
  input.upshift = true;
  input.downshift = false;
  input.reset = true;
  input.enter_exit_box = true;
  EXPECT_DOUBLE_EQ(input.throttle, 1.0);
  EXPECT_DOUBLE_EQ(input.brake, 0.5);
  EXPECT_DOUBLE_EQ(input.steering, -0.8);
  EXPECT_TRUE(input.upshift);
  EXPECT_FALSE(input.downshift);
  EXPECT_TRUE(input.reset);
  EXPECT_TRUE(input.enter_exit_box);
}
