#pragma once

#include "common.h"
#include "simulation/simulation.h"
#include "telemetry/telemetry.h"
#include "../engine/input/input_manager.h"
#include "game_state.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

// Project 0 — gameplay loop (console/headless)
// Namespace: p0::gameplay
namespace p0::gameplay {

struct LapTime {
  double lap_time = 0.0;
  bool valid = true;
};

struct GameplayState {
  bool running = true;
  bool reset_requested = false;
  int current_lap = 0;
  double best_lap_time = 0.0;
  double current_lap_time = 0.0;
  std::vector<LapTime> lap_times;
  bool off_track_warning = false;
  int off_track_frames = 0;
};

class Gameplay {
 public:
  Gameplay(simulation::Simulation& sim, telemetry::Telemetry& tel, std::unique_ptr<input::InputManager> input_manager);
  ~Gameplay() = default;

  void run();
  void stop() { state_.running = false; }

  const GameplayState& state() const { return state_; }
  void set_off_track_warning(bool warning) { state_.off_track_warning = warning; }

  void update_lap_timing(const simulation::SimulationResult& result);

  void handle_menu_input(const input::InputState& input);
  void handle_countdown();
  void handle_racing(const simulation::SimulationResult& result);
  void show_results();
  void save_best_times();
  void load_best_times();
  void reset_race();

 private:
  input::InputState poll_input();
  void render_console(const simulation::SimulationResult& result);
  void render_menu();
  void render_countdown();
  void render_results();

 private:
  simulation::Simulation& sim_;
  telemetry::Telemetry& tel_;
  std::unique_ptr<input::InputManager> input_manager_;
  GameplayState state_;
  double last_lap_distance_ = 0.0;
  double last_sim_time_ = 0.0;
};

}

