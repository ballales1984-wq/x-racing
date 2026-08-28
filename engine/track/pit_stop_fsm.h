#pragma once

#include "race_config.h"
#include "track_data.h"
#include "pit_lane.h"
#include <string>
#include <vector>

namespace p0::track {

// ---------------------------------------------------------------------------
// PitStopFSM — finite state machine for a single car's pit stop
// ---------------------------------------------------------------------------
class PitStopFSM {
 public:
  explicit PitStopFSM(int car_id, const PitLaneSystem& pit_system);

  void request_stop(p0::race::TireCompound new_tire, bool refuel, bool tires, bool repair);
  void update(double timestamp, double car_track_pos_m, double car_speed_m_s);
  void abandon();

  p0::race::PitStopState state() const { return state_; }
  bool is_complete() const;
  bool is_active() const;
  bool has_violation() const;

  const CarPitState& car_state() const { return car_state_; }
  CarPitState& mutable_car_state() { return car_state_; }

  std::string state_name() const;
  std::string debug_string() const;

 private:
  void transition_to(p0::race::PitStopState new_state, double timestamp);
  void check_box_alignment(double timestamp, double car_track_pos_m);
  void check_release_conditions(double timestamp);

  int car_id_;
  PitLaneSystem const* pit_system_;
  p0::race::PitStopState state_ = p0::race::PitStopState::NONE;
  CarPitState car_state_;
  double state_entry_time_ = 0.0;
  bool service_requested_ = false;
  p0::race::TireCompound requested_tire_ = p0::race::TireCompound::MEDIUM;
  bool request_refuel_ = false;
  bool request_tires_ = false;
  bool request_repair_ = false;
  double service_started_ = 0.0;
  double service_duration_ = 0.0;
};

// ---------------------------------------------------------------------------
// PitStopManager — manages all pit stops for a race
// ---------------------------------------------------------------------------
class PitStopManager {
 public:
  explicit PitStopManager(const PitLaneDefinition& pit_def);

  void register_car(int car_id);
  void unregister_car(int car_id);

  bool request_pit_stop(int car_id,
                        p0::race::TireCompound new_tire,
                        bool refuel,
                        bool change_tires,
                        bool repair);

  void update(double timestamp,
              const std::unordered_map<int, double>& car_positions,
              const std::unordered_map<int, double>& car_speeds);

  p0::race::PitStopState car_state(int car_id) const;
  bool is_pit_stop_active(int car_id) const;
  int active_pit_stops() const;

  const std::vector<SpeedViolation>& violations() const { return violations_; }
  std::vector<SpeedViolation> pop_served_violations();

  PitLaneSystem& pit_system() { return pit_system_; }
  const PitLaneSystem& pit_system() const { return pit_system_; }

 private:
  PitLaneSystem pit_system_;
  std::unordered_map<int, PitStopFSM> fsm_map_;
  std::vector<SpeedViolation> violations_;
};

}
