#pragma once

#include "common.h"
#include "ai/ai_driver.h"
#include "simulation/simulation.h"
#include "vehicle/vehicle.h"
#include "track/track.h"
#include <string>
#include <vector>

namespace p0::ai {

struct OpponentState {
  int car_id = -1;
  std::string name;
  AIDifficulty difficulty = AIDifficulty::MEDIUM;
  double distance_along_track = 0.0;
  double speed_m_s = 0.0;
  int lap = 0;
  bool finished = false;
  double total_time = 0.0;
  double best_lap_time = 0.0;
};

class Opponent {
 public:
  explicit Opponent(const AIDriverParams& params = {}, const std::string& name = "AI");

  void set_track(const track::Track& track);
  void set_racing_line(const std::vector<track::RacingLineSample>& samples);
  void reset(const vehicle::VehicleState& state);

  input::InputState update(const vehicle::VehicleState& state, double delta_time);

  const AIDriver& driver() const { return driver_; }
  AIDriver& driver() { return driver_; }

  OpponentState state() const { return state_; }
  void set_state(const OpponentState& s) { state_ = s; }

 private:
  AIDriver driver_;
  OpponentState state_;
};

class OpponentManager {
 public:
  explicit OpponentManager(const track::Track* track = nullptr);

  void set_track(const track::Track& track);
  void set_racing_line(const std::vector<track::RacingLineSample>& samples);

  int add_opponent(const AIDriverParams& params, const std::string& name);
  void remove_opponent(int car_id);

  void reset_all(const std::vector<vehicle::VehicleState>& states);
  std::vector<input::InputState> update_all(const std::vector<vehicle::VehicleState>& states,
                                            double delta_time);

  const Opponent* get(int car_id) const;
  Opponent* get(int car_id);

  std::vector<const Opponent*> opponents() const;
  int count() const { return static_cast<int>(opponents_.size()); }

  void clear();

 private:
  const track::Track* track_ = nullptr;
  std::vector<track::RacingLineSample> racing_line_;
  std::vector<Opponent> opponents_;
  int next_id_ = 1;
};

}
