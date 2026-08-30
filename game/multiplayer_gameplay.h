#pragma once

#include "common.h"
#include "network/lobby.h"
#include "network/network_session.h"
#include "network/network_protocol.h"
#include "simulation/simulation_world.h"
#include "game/game_state.h"
#include "vehicle/vehicle.h"
#include "input/input.h"
#include "input/input_manager.h"
#include "input/platform/windows_input.h"
#include "telemetry/telemetry.h"
#include "track/track.h"
#include "ai/ai_driver.h"
#include "tracking/tracking_system.h"
#include "tracking/physics_trajectory.h"
#include "tracking/coordinate_converter.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace p0::gameplay {

struct MultiplayerConfig {
  bool is_host = true;
  std::string host_address = "127.0.0.1";
  int local_car_id = 0;
  int max_players = p0::network::kMaxPlayers;
  int lap_count = 3;
  bool fill_with_ai = true;
  p0::ai::AIDifficulty ai_difficulty = p0::ai::AIDifficulty::MEDIUM;
};

enum class MultiplayerState {
  MENU,
  LOBBY,
  COUNTDOWN,
  RACING,
  PAUSED,
  RESULTS
};

struct MultiplayerResults {
  bool completed = false;
  double best_lap_time = 0.0;
  double total_time = 0.0;
  int completed_laps = 0;
  int position = 0;
  std::vector<double> lap_times;
};

class MultiplayerGameplay {
 public:
  using OnStateChange = std::function<void(MultiplayerState)>;
  using OnResults = std::function<void(const MultiplayerResults&)>;

  MultiplayerGameplay();
  ~MultiplayerGameplay();

  bool initialize(const MultiplayerConfig& config);
  void shutdown();

  void update(double delta_time);
  void handle_input(const input::InputState& input);

  void host_race(const std::string& host_name);
  void join_race(const std::string& host_address);
  void leave_race();
  void start_countdown();

  MultiplayerState state() const { return state_; }
  const MultiplayerResults& results() const { return results_; }
  const simulation::SimulationWorld& world() const { return world_; }
  simulation::SimulationWorld& world() { return world_; }
  network::Lobby& lobby() { return lobby_; }
  const network::Lobby& lobby() const { return lobby_; }
  const vehicle::VehicleState* local_state() const;

  bool is_host() const { return config_.is_host; }
  int local_car_id() const { return config_.local_car_id; }

  void set_on_state_change(OnStateChange cb) { on_state_change_ = cb; }
  void set_on_results(OnResults cb) { on_results_ = cb; }

  std::string race_status_text() const;

 private:
  MultiplayerConfig config_;
  MultiplayerState state_ = MultiplayerState::MENU;
  MultiplayerResults results_;
  simulation::SimulationWorld world_;
  network::Lobby lobby_;
  std::unique_ptr<input::InputManager> local_input_;
  std::unique_ptr<telemetry::Telemetry> telemetry_;
  std::unique_ptr<p0::tracking::CoordinateConverter> tracking_converter_;
  std::unique_ptr<p0::tracking::PhysicsTrajectory> local_trajectory_;

  double countdown_start_time_ = 0.0;
  double countdown_duration_ = 3.0;
  bool countdown_finished_ = false;

  OnStateChange on_state_change_;
  OnResults on_results_;

  void update_menu(double delta_time);
  void update_lobby(double delta_time);
  void update_countdown(double delta_time);
  void update_racing(double delta_time);
  void update_results(double delta_time);
  void check_local_finish();
  void build_telemetry();
  void set_state(MultiplayerState new_state);
};

}
