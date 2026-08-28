#pragma once

#include "race_config.h"
#include "track_data.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace p0::track {

using p0::race::TireCompound;
using p0::race::PitStopState;
using p0::race::CarFuelState;
using p0::race::CarTireState;

// ---------------------------------------------------------------------------
// PitServiceResult — outcome of a single service operation
// ---------------------------------------------------------------------------
struct PitServiceResult {
  bool refueled = false;
  double fuel_added_l = 0.0;
  double refuel_time_s = 0.0;
  bool tires_changed = false;
  p0::race::TireCompound new_compound = p0::race::TireCompound::MEDIUM;
  double tire_change_time_s = 0.0;
  bool repaired = false;
  double repair_time_s = 0.0;
  double total_service_time_s = 0.0;
};

// ---------------------------------------------------------------------------
// SpeedViolation — recorded infraction
// ---------------------------------------------------------------------------
struct SpeedViolation {
  int car_id = 0;
  double recorded_speed_m_s = 0.0;
  double limit_m_s = 0.0;
  double timestamp = 0.0;
  double track_position_m = 0.0;
  p0::race::ViolationType type = p0::race::ViolationType::PIT_SPEED_EXCEEDED;
  p0::race::PenaltyType penalty = p0::race::PenaltyType::DRIVE_THROUGH;
  bool served = false;
};

// ---------------------------------------------------------------------------
// CarPitState — dynamic pit state for a single car
// ---------------------------------------------------------------------------
struct CarPitState {
  int car_id = 0;
  p0::race::PitStopState state = p0::race::PitStopState::NONE;
  int assigned_box_id = -1;
  double pit_entry_time = 0.0;
  double box_stop_time = 0.0;
  double service_start_time = 0.0;
  double service_end_time = 0.0;
  double pit_exit_time = 0.0;
  int pit_stops_completed = 0;
  std::vector<SpeedViolation> violations;
  PitServiceResult last_service;

  double speed_zone_entry_time = 0.0;
  double speed_zone_entry_position_m = 0.0;
  bool speed_zone_entered = false;
};

// ---------------------------------------------------------------------------
// PitLaneSystem — runtime pit lane management
// ---------------------------------------------------------------------------
class PitLaneSystem {
 public:
  explicit PitLaneSystem(const PitLaneDefinition& def);

  int assign_box(int car_id, int team_id);
  void release_box(int car_id);
  int find_box_for_team(int team_id) const;
  int find_free_box() const;
  bool is_box_free(int box_id) const;

  void on_cross_speed_start(int car_id, double timestamp, double track_pos);
  void on_cross_speed_end(int car_id, double timestamp, double track_pos);
  std::vector<SpeedViolation> process_speed_violations();
  const std::vector<SpeedViolation>& violations() const { return violations_; }

  CarPitState& car_state(int car_id);
  const CarPitState& car_state(int car_id) const;
  void reset_car(int car_id);

  int cars_in_pit_lane() const;
  int cars_stopped() const;
  bool can_enter_pit_lane() const;
  bool can_stop_at_box() const;

  PitServiceResult calculate_service(
      const p0::race::CarFuelState& fuel,
      const p0::race::CarTireState& tires,
      p0::race::TireCompound new_tire,
      bool do_refuel,
      bool do_tires,
      bool do_repair,
      double damage_hp,
      double refuel_rate_l_s,
      double tire_change_time_s,
      double repair_rate_hp_s) const;

  const PitLaneDefinition& definition() const { return def_; }

 private:
  PitLaneDefinition def_;
  std::unordered_map<int, CarPitState> car_states_;
  std::vector<SpeedViolation> violations_;

  double average_speed_m_s(double dist, double time) const;
};

}
