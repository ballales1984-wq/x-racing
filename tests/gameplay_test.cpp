// Project 0 — unit tests for gameplay loop and lap timing
#include <gtest/gtest.h>
#include "game/gameplay.h"
#include "simulation/simulation.h"
#include "telemetry/telemetry.h"
#include "track/track.h"
#include "input/input.h"
#include <memory>
#include <windows.h>
#include <chrono>
#include <fstream>
#include <filesystem>

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

// FakeInputManager with configurable escape key behavior
class GamepadFakeInputManager : public input::InputManager {
 public:
  explicit GamepadFakeInputManager(input::InputState fixed_input, bool escape_pressed = false)
      : fixed_(fixed_input), escape_pressed_(escape_pressed) {}

  input::InputState poll() override { return fixed_; }
  bool is_key_down(int key_code) override {
    if (key_code == VK_ESCAPE) return escape_pressed_;
    return false;
  }

 private:
  input::InputState fixed_;
  bool escape_pressed_;
};

// reset_race should reset all state to defaults
TEST(Gameplay, ResetRaceClearsState) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Set some state first
  simulation::SimulationResult result;
  result.time = 10.0;
  result.state.lap = 0;
  result.state.distance_along_track = 50.0;
  gp.update_lap_timing(result);

  gp.reset_race();
  EXPECT_TRUE(gp.state().running);
  EXPECT_EQ(gp.state().current_lap, 0);
  EXPECT_DOUBLE_EQ(gp.state().best_lap_time, 0.0);
  EXPECT_DOUBLE_EQ(gp.state().current_lap_time, 0.0);
  EXPECT_TRUE(gp.state().lap_times.empty());
  EXPECT_FALSE(gp.state().off_track_warning);
  EXPECT_EQ(gp.state().off_track_frames, 0);
}

// handle_menu_input should cycle track options with throttle (W)
TEST(Gameplay, MenuInputCyclesTracks) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Reset menu state
  gp.menu().current_menu_index = 0;
  gp.menu().selected_track = 0;

  input::InputState up_input{};
  up_input.throttle = 1.0;
  gp.handle_menu_input(up_input);

  EXPECT_EQ(gp.menu().selected_track, 1);
}

// handle_menu_input should cycle lap options with throttle (W) when on laps group
TEST(Gameplay, MenuInputCyclesLaps) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  gp.menu().current_menu_index = 1;
  gp.menu().selected_laps = 1;

  input::InputState down_input{};
  down_input.throttle = 1.0;
  gp.handle_menu_input(down_input);

  EXPECT_EQ(gp.menu().selected_laps, 3);
}

// handle_menu_input should switch menu group with upshift
TEST(Gameplay, MenuInputUpshiftAdvances) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  gp.menu().current_menu_index = 0;
  gp.menu().selected_track = 0;

  input::InputState input{};
  input.upshift = true;
  gp.handle_menu_input(input);

  EXPECT_EQ(gp.menu().current_menu_index, 1);
}

// handle_menu_input should toggle menu group with box (B) key
TEST(Gameplay, MenuInputBoxTogglesGroup) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  gp.menu().current_menu_index = 1;

  input::InputState input{};
  input.enter_exit_box = true;
  gp.handle_menu_input(input);

  EXPECT_EQ(gp.menu().current_menu_index, 0);
}

// handle_menu_input should start countdown on reset
TEST(Gameplay, MenuInputResetStartsCountdown) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  gp.set_phase(gameplay::GameState::MENU);

  input::InputState input{};
  input.reset = true;
  gp.handle_menu_input(input);

  EXPECT_EQ(gp.phase(), gameplay::GameState::COUNTDOWN);
  EXPECT_FALSE(gp.countdown().finished);
  EXPECT_EQ(gp.countdown().last_number, -1);
}

// handle_countdown should transition to racing when countdown finishes
TEST(Gameplay, CountdownTransitionsToRacing) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Set countdown start time in the past so countdown has finished
  gp.countdown().start_time = -10.0;
  gp.countdown().finished = false;
  gp.set_phase(gameplay::GameState::COUNTDOWN);

  gp.handle_countdown();

  EXPECT_TRUE(gp.countdown().finished);
  EXPECT_EQ(gp.phase(), gameplay::GameState::RACING);
}

// handle_countdown should show number 3 at start
TEST(Gameplay, CountdownShowsNumber) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Set countdown start time to now so elapsed ≈ 0
  gp.countdown().start_time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
  gp.countdown().finished = false;

  gp.handle_countdown();
  EXPECT_EQ(gp.countdown().last_number, 3);
}

// handle_racing should set off_track_warning when result.off_track
TEST(Gameplay, RacingSetsOffTrackWarning) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  simulation::SimulationResult result{};
  result.time = 1.0;
  result.state.lap = 0;
  result.off_track = true;

  gp.handle_racing(result);
  EXPECT_TRUE(gp.state().off_track_warning);
  EXPECT_EQ(gp.state().off_track_frames, 1);
}

// handle_racing should reset off_track_frames when not off track
TEST(Gameplay, RacingResetsOffTrackFrames) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // First set off track
  simulation::SimulationResult result{};
  result.time = 1.0;
  result.state.lap = 0;
  result.off_track = true;
  gp.handle_racing(result);
  EXPECT_EQ(gp.state().off_track_frames, 1);

  // Then back on track
  result.off_track = false;
  gp.handle_racing(result);
  EXPECT_EQ(gp.state().off_track_frames, 0);
}

// handle_racing should trigger respawn on collision
TEST(Gameplay, RacingRespawnOnCollision) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  simulation::SimulationResult result{};
  result.time = 1.0;
  result.state.lap = 0;
  result.collision = true;

  gp.handle_racing(result);
  EXPECT_EQ(gp.state().off_track_frames, 0);
}

// handle_racing should complete race when lap count is reached
TEST(Gameplay, RacingCompletesWhenLapCountReached) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Set selected_laps to 1 and current_lap to 1 so race completes
  gp.menu().selected_laps = 1;

  simulation::SimulationResult result{};
  result.time = 50.0;
  result.state.lap = 0;
  gp.update_lap_timing(result);

  result.time = 50.001;
  result.state.lap = 1;
  result.state.distance_along_track = track.length() + 10.0;

  gp.handle_racing(result);

  EXPECT_EQ(gp.phase(), gameplay::GameState::RESULTS);
  EXPECT_TRUE(gp.results().completed);
  EXPECT_EQ(gp.results().completed_laps, 1);
}

// save_best_times / load_best_times round-trip
TEST(Gameplay, SaveLoadBestTimesRoundTrip) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Populate results
  gp.results().best_lap_time = 12.5;
  gp.results().total_time = 37.5;
  gp.results().completed_laps = 3;
  gp.results().lap_times = {12.5, 13.0, 12.0};
  gp.results().lap_valid = {true, true, true};
  gp.results().completed = true;

  gp.save_best_times();

  // Reset and reload
  gp.results() = gameplay::RaceResults{};
  gp.load_best_times();

  EXPECT_NEAR(gp.results().best_lap_time, 12.5, 1e-3);
  EXPECT_NEAR(gp.results().total_time, 37.5, 1e-3);
  EXPECT_EQ(gp.results().completed_laps, 3);
  EXPECT_TRUE(gp.results().completed);
  ASSERT_EQ(gp.results().lap_times.size(), 3u);
  EXPECT_NEAR(gp.results().lap_times[0], 12.5, 1e-3);
  ASSERT_EQ(gp.results().lap_valid.size(), 3u);
  EXPECT_TRUE(gp.results().lap_valid[0]);
}

// load_best_times should handle missing file gracefully
TEST(Gameplay, LoadBestTimesMissingFile) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Ensure the file doesn't exist
  std::filesystem::remove("data/best_times.json");

  gp.results() = gameplay::RaceResults{};
  // Calling load_best_times with no file should not crash
  gp.load_best_times();
  EXPECT_FALSE(gp.results().completed);
}

// update_lap_timing should handle off_track_warning invalidation
TEST(Gameplay, UpdateLapTimingInvalidatesWithOffTrack) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Accumulate time during lap 0 with off-track warning
  simulation::SimulationResult result{};
  result.time = 20.0;
  result.state.lap = 0;
  result.state.distance_along_track = 10.0;
  gp.set_off_track_warning(true);
  gp.update_lap_timing(result);

  // Complete lap
  result.time = 20.001;
  result.state.lap = 1;
  result.state.distance_along_track = track.length() + 10.0;
  gp.update_lap_timing(result);

  ASSERT_EQ(gp.state().lap_times.size(), 1u);
  EXPECT_FALSE(gp.state().lap_times[0].valid);
}

// update_lap_timing should not set best_lap_time for invalid laps
TEST(Gameplay, UpdateLapTimingNoBestForInvalid) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  simulation::SimulationResult result{};
  result.time = 20.0;
  result.state.lap = 0;
  gp.update_lap_timing(result);

  result.time = 20.001;
  result.state.lap = 1;
  result.state.distance_along_track = track.length() + 10.0;
  gp.set_off_track_warning(true);
  gp.update_lap_timing(result);

  EXPECT_EQ(gp.state().lap_times.size(), 1u);
  EXPECT_FALSE(gp.state().lap_times[0].valid);
  EXPECT_DOUBLE_EQ(gp.state().best_lap_time, 0.0);
}

// Gameplay should use stored best_lap_time when results best is empty
TEST(Gameplay, HandleRacingUsesStoredBestLap) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);
  telemetry::Telemetry tel;
  auto input_mgr = std::make_unique<GamepadFakeInputManager>(input::InputState{});
  gameplay::Gameplay gp(sim, tel, std::move(input_mgr));

  // Reset global state
  gp.results() = gameplay::RaceResults{};
  gp.menu().selected_laps = 1;

  // Accumulate time during lap 0
  simulation::SimulationResult result{};
  result.time = 30.0;
  result.state.lap = 0;
  result.state.distance_along_track = 10.0;
  gp.update_lap_timing(result);

  // Complete lap and trigger race completion via handle_racing
  result.time = 30.001;
  result.state.lap = 1;
  result.state.distance_along_track = track.length() + 10.0;
  gp.handle_racing(result);

  // results.best_lap_time should match gp state's best_lap_time
  EXPECT_NEAR(gp.results().best_lap_time, gp.state().best_lap_time, 1e-9);
  EXPECT_TRUE(gp.results().completed);
}
