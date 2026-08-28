#pragma once

#include "race_config.h"
#include "track_data.h"
#include "pit_stop_fsm.h"
#include "validation.h"
#include "common.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace p0::track {

// ---------------------------------------------------------------------------
// RaceManager — central orchestrator for race runtime
// ---------------------------------------------------------------------------
class RaceManager {
 public:
  RaceManager(const TrackData& track, const p0::race::RaceDefinition& race);

  bool initialize(const std::vector<p0::race::CarAssignment>& assignments,
                  const std::vector<p0::race::TeamDefinition>& teams);

  void start_race();
  void update(double timestamp,
              const std::unordered_map<int, Vec2>& car_positions,
              const std::unordered_map<int, double>& car_speeds,
              const std::unordered_map<int, double>& car_fuel,
              const std::unordered_map<int, p0::race::TireCompound>& car_tires);

  p0::race::RaceSessionState session_state() const { return session_state_; }
  int current_lap() const { return current_lap_; }
  double race_time() const { return race_time_; }

  bool request_pit_stop(int car_id,
                        p0::race::TireCompound new_tire = p0::race::TireCompound::MEDIUM,
                        bool refuel = true,
                        bool change_tires = true,
                        bool repair = false);

  p0::race::PitStopState car_pit_state(int car_id) const;
  bool car_has_pit_violation(int car_id) const;
  std::vector<p0::track::SpeedViolation> pop_pending_violations();

  const std::vector<p0::race::ValidationIssue>& validation_issues() const { return validation_issues_; }
  bool is_valid() const;

  const TrackData& track_data() const { return track_; }
  const p0::race::RaceDefinition& race_definition() const { return race_; }
  PitStopManager& pit_manager() { return pit_manager_; }
  const PitStopManager& pit_manager() const { return pit_manager_; }

  std::string debug_report() const;

 private:
  void update_session_state(double timestamp);
  void update_lap_counters(const std::unordered_map<int, Vec2>& car_positions);
  void check_fuel_strategy(double timestamp);
  void validate_setup();

  const TrackData& track_;
  const p0::race::RaceDefinition& race_;
  p0::race::RaceSessionState session_state_ = p0::race::RaceSessionState::PREGAME;
  int current_lap_ = 0;
  double race_time_ = 0.0;
  double session_start_time_ = 0.0;
  bool initialized_ = false;

  PitStopManager pit_manager_;
  std::vector<p0::race::CarAssignment> assignments_;
  std::vector<p0::race::TeamDefinition> teams_;

  std::unordered_map<int, double> car_last_lap_pos_;
  std::unordered_map<int, int> car_lap_count_;
  std::unordered_map<int, double> car_fuel_consumption_;
  std::vector<p0::race::ValidationIssue> validation_issues_;

  static constexpr double kFuelWarningThreshold = 0.15;
  static constexpr double kTireWarningThreshold = 0.60;
};

}

