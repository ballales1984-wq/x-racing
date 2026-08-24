#pragma once

#include "common.h"
#include "simulation/simulation.h"
#include "telemetry/telemetry.h"
#include "../engine/input/input_manager.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

// Project 0 — gameplay loop (console/headless)
// Namespace: p0::gameplay
namespace p0::gameplay {

// One recorded lap time and whether it counts as valid.
struct LapTime {
  double lap_time = 0.0;
  bool valid = true;
};

// Mutable state of a gameplay session.
struct GameplayState {
  bool running = true;
  bool reset_requested = false;
  int current_lap = 0;
  double best_lap_time = 0.0;
  double current_lap_time = 0.0;
  std::vector<LapTime> lap_times;
  bool off_track_warning = false;
};

class Gameplay {
 public:
  Gameplay(simulation::Simulation& sim, telemetry::Telemetry& tel, std::unique_ptr<input::InputManager> input_manager);
  ~Gameplay() = default;

  void run();
  void stop() { state_.running = false; }

  const GameplayState& state() const { return state_; }

 private:
  // Poll the input manager for the latest driver input.
  input::InputState poll_input();
  // Render the current state to the console HUD.
  void render_console(const simulation::SimulationResult& result);
  // Track lap changes and update timing/standings.
  void update_lap_timing(const simulation::SimulationResult& result);

  simulation::Simulation& sim_;
  telemetry::Telemetry& tel_;
  std::unique_ptr<input::InputManager> input_manager_;
  GameplayState state_;
  double last_lap_distance_ = 0.0;
};

}

