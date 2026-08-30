#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include "input/input.h"
#include "input/input_manager.h"
#include "simulation/simulation.h"
#include "track/race_config.h"
#include "tracking/tracking_system.h"
#include <memory>
#include <unordered_map>
#include <functional>

namespace p0::simulation {

using p0::vehicle::VehicleState;
using p0::race::RaceSessionState;

enum class DriverType : uint8_t {
  HUMAN = 0,
  AI
};

struct CarInstance {
  int car_id = -1;
  DriverType driver_type = DriverType::HUMAN;
  int player_id = -1;
  std::string name;
  std::unique_ptr<Simulation> simulation;
  input::InputState pending_input;
  bool has_input = false;
  VehicleState last_state;
  double distance_along_track = 0.0;
  int lap = 0;
  int prev_lap = 0;
  bool finished = false;
  double finish_time = 0.0;
  uint32_t input_sequence = 0;
  bool in_pit = false;
  double lap_start_time = 0.0;  // sim time when current lap began (for lap timing)
};

struct WorldUpdateResult {
  bool race_started = false;
  bool race_finished = false;
  int leader_car_id = -1;
  double leader_time = 0.0;
  p0::race::RaceSessionState session_state = p0::race::RaceSessionState::PREGAME;
  int current_lap = 0;
  double race_time = 0.0;
};

class SimulationWorld {
 public:
  using OnRaceFinished = std::function<void(int car_id, double time, int laps)>;
  using OnLapCompleted = std::function<void(int car_id, int lap, double lap_time)>;

  using RaceUpdateCb = std::function<void(double, const std::unordered_map<int, Vec2>&,
                                          const std::unordered_map<int, double>&,
                                          const std::unordered_map<int, double>&,
                                          const std::unordered_map<int, race::TireCompound>&)>;

  SimulationWorld(const SimulationWorld&) = delete;
  SimulationWorld& operator=(const SimulationWorld&) = delete;
  SimulationWorld(SimulationWorld&&) = default;
  SimulationWorld& operator=(SimulationWorld&&) = default;

  SimulationWorld();
  ~SimulationWorld();

  bool initialize(const track::Track& track);
  void shutdown();

  int add_car(const vehicle::VehicleParams& params, DriverType driver_type, int player_id, const std::string& name);
  void remove_car(int car_id);
  void reset_car(int car_id, const vehicle::VehicleState& initial_state);
  void reset_all(const std::unordered_map<int, vehicle::VehicleState>& initial_states);

  void set_input(int car_id, const input::InputState& input);
  void set_ai_input(int car_id, const input::InputState& ai_input);

  WorldUpdateResult update(double delta_time, double timestamp);
  WorldUpdateResult update_with_race_manager(double delta_time, double timestamp, RaceUpdateCb race_update_cb);

  const CarInstance* get_car(int car_id) const;
  CarInstance* get_car(int car_id);
  const vehicle::VehicleState* get_state(int car_id) const;
  const std::unordered_map<int, CarInstance>& cars() const { return cars_; }
  int car_count() const { return static_cast<int>(cars_.size()); }
  int active_car_count() const;
  bool has_car(int car_id) const { return cars_.count(car_id) > 0; }
  int local_car_id() const { return local_car_id_; }
  void set_local_car_id(int id) { local_car_id_ = id; }

  void set_track(const track::Track& track);
  const track::Track& track() const;

  void set_tracking_system(std::unique_ptr<p0::tracking::TrackingSystem> tracking);
  p0::tracking::TrackingSystem* tracking_system() const { return tracking_system_.get(); }

  void set_total_laps(int laps) { total_laps_ = laps; }
  int total_laps() const { return total_laps_; }

  void set_on_race_finished(OnRaceFinished cb) { on_race_finished_ = cb; }
  void set_on_lap_completed(OnLapCompleted cb) { on_lap_completed_ = cb; }

 private:
  std::unordered_map<int, CarInstance> cars_;
  const track::Track* track_ = nullptr;
  std::unique_ptr<p0::tracking::TrackingSystem> tracking_system_;
  int local_car_id_ = 0;
  int next_car_id_ = 0;
  double total_race_time_ = 0.0;
  bool race_started_ = false;
  int total_laps_ = 0;

  OnRaceFinished on_race_finished_;
  OnLapCompleted on_lap_completed_;

  void check_lap_completions();
  void check_race_finish();
};

}
