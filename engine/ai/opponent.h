// Project 0 — AI opponent wrapper (AIDriver + state management)
// Namespace: p0::ai
#pragma once

#include "common.h"
#include "ai/ai_driver.h"
#include "simulation/simulation.h"
#include "vehicle/vehicle.h"
#include "track/track.h"
#include <string>
#include <vector>

namespace p0::ai {

// Runtime state snapshot for an AI opponent (exposed to race manager).
struct OpponentState {
  int car_id = -1;
  std::string name;
  AIDifficulty difficulty = AIDifficulty::MEDIUM;
  double distance_along_track = 0.0;  // m
  double speed_m_s = 0.0;
  int lap = 0;
  bool finished = false;
  double total_time = 0.0;  // s
  double best_lap_time = 0.0;  // s
};

// Wraps an AIDriver with opponent-specific state.
// Produces InputState each frame for injection into the simulation.
class Opponent {
 public:
  explicit Opponent(const AIDriverParams& params = {}, const std::string& name = "AI");

  // Configure track geometry and racing line.
  void set_track(const track::Track& track);
  void set_racing_line(const std::vector<track::RacingLineSample>& samples);
  // Reset vehicle state at session start or respawn.
  void reset(const vehicle::VehicleState& state);

  // Compute AI input for the next physics tick.
  input::InputState update(const vehicle::VehicleState& state, double delta_time);

  const AIDriver& driver() const { return driver_; }
  AIDriver& driver() { return driver_; }

  // Current runtime state (updated each frame by the manager).
  OpponentState state() const { return state_; }
  void set_state(const OpponentState& s) { state_ = s; }

 private:
  AIDriver driver_;
  OpponentState state_;
};

// Manages a collection of Opponent instances.
// Provides batch reset/update for all opponents in a race session.
class OpponentManager {
 public:
  explicit OpponentManager(const track::Track* track = nullptr);

  // Bind track geometry and racing line to all existing opponents.
  void set_track(const track::Track& track);
  void set_racing_line(const std::vector<track::RacingLineSample>& samples);

  // Add a new opponent and return its assigned car ID.
  int add_opponent(const AIDriverParams& params, const std::string& name);
  void remove_opponent(int car_id);

  // Reset all opponents with fresh vehicle states (session start / respawn).
  void reset_all(const std::vector<vehicle::VehicleState>& states);
  // Update all opponents and return their inputs for the simulation.
  std::vector<input::InputState> update_all(const std::vector<vehicle::VehicleState>& states,
                                            double delta_time);

  const Opponent* get(int car_id) const;
  Opponent* get(int car_id);

  // All opponents currently managed by this instance.
  std::vector<const Opponent*> opponents() const;
  int count() const { return static_cast<int>(opponents_.size()); }

  void clear();

  private:
   const track::Track* track_ = nullptr;
   std::vector<track::RacingLineSample> racing_line_;
   std::vector<Opponent> opponents_;
   int next_id_ = 1;  // next car ID to assign
};

}
