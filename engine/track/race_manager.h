#pragma once

#include "race_config.h"
#include "race_results.h"
#include "track_data.h"
#include "pit_stop_fsm.h"
#include "validation.h"
#include "standings.h"
#include "tracking/lap_system.h"
#include "tracking/checkpoint.h"
#include "tracking/track_position.h"
#include "common.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <deque>

namespace p0::track {

class RaceManager {
 public:
  RaceManager(const TrackData& track, const p0::race::RaceDefinition& race);

  bool initialize(const std::vector<p0::race::CarAssignment>& assignments,
                  const std::vector<p0::race::TeamDefinition>& teams);

  void start_race();
  void start_countdown();
  void update(double timestamp,
              const std::unordered_map<int, Vec2>& car_positions,
              const std::unordered_map<int, double>& car_speeds,
              const std::unordered_map<int, double>& car_distances,
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

  // --- Phase 4: Countdown & Events ---
  p0::race::FlagState flag_state() const { return flag_state_; }
  std::vector<p0::race::RaceEvent> drain_events();

  // --- Phase 4: Standings ---
  std::vector<CarStandingsEntry> current_standings() const;
  CarStandingsEntry car_standings(int car_id) const;

  // --- Phase 4: Lap Validation ---
  bool is_lap_valid(int car_id) const;
  std::vector<p0::track::LapTimeEntry> car_lap_times(int car_id) const;

  // --- Phase 4: Penalties ---
  void issue_penalty(int car_id, p0::race::ViolationType type);
  bool car_has_penalty(int car_id) const;
  std::vector<p0::race::ViolationType> car_active_penalties(int car_id) const;

 private:
  void update_countdown(double timestamp);
  void update_lap_systems(double timestamp);
  void update_track_limits(double timestamp, const std::unordered_map<int, Vec2>& car_positions);
  void update_jump_start(double timestamp, const std::unordered_map<int, double>& car_speeds);
  void update_penalty_serving();
  void update_lap_counters();
  void update_standings();
  void update_session_state(double timestamp);
  void check_fuel_strategy(double timestamp);
  void validate_setup();

  void emit_event(p0::race::RaceEventType type, int car_id = 0, const std::string& msg = "");

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

  std::unordered_map<int, int> car_lap_count_;
  std::unordered_map<int, double> car_fuel_consumption_;
  std::vector<p0::race::ValidationIssue> validation_issues_;

  // --- Phase 4: Countdown ---
  double countdown_start_time_ = 0.0;
  bool countdown_active_ = false;

  // --- Phase 4: Lap Systems ---
  std::map<int, std::unique_ptr<p0::tracking::LapSystem>> car_lap_systems_;
  p0::tracking::CheckpointSystem checkpoint_system_;
  std::unordered_map<int, std::vector<LapTimeEntry>> car_lap_times_;
  std::unordered_map<int, bool> car_lap_valid_;
  std::unordered_map<int, double> car_last_distance_;

  // --- Phase 4: Track Limits ---
  std::unordered_map<int, bool> car_off_track_;
  std::unordered_map<int, double> car_off_track_start_;
  std::unordered_map<int, int> car_track_limits_strikes_;

  // --- Phase 4: Penalties ---
  std::unordered_map<int, std::vector<p0::race::ViolationType>> car_penalties_;
  std::unordered_map<int, bool> car_penalty_served_;

  // --- Phase 4: Standings ---
  StandingsTracker standings_tracker_;
  std::unordered_map<int, double> car_finished_times_;

  // --- Phase 4: Flags ---
  p0::race::FlagState flag_state_ = p0::race::FlagState::GREEN;

  // --- Phase 4: Events ---
  std::deque<p0::race::RaceEvent> events_;

  static constexpr double kFuelWarningThreshold = 0.15;
  static constexpr double kTireWarningThreshold = 0.60;
};

}
