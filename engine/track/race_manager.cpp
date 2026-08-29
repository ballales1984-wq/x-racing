#include "race_manager.h"
#include <algorithm>
#include <sstream>

namespace p0::track {

using p0::race::RaceSessionState;
using p0::race::PitStopState;

RaceManager::RaceManager(const TrackData& track, const p0::race::RaceDefinition& race)
    : track_(track), race_(race), pit_manager_(race_.track_id == track.track_id
                                                   ? track.pit_lane
                                                   : PitLaneDefinition{}) {}

bool RaceManager::initialize(const std::vector<p0::race::CarAssignment>& assignments,
                             const std::vector<p0::race::TeamDefinition>& teams) {
  assignments_ = assignments;
  teams_ = teams;

  if (assignments_.empty()) return false;

  for (const auto& a : assignments_) {
    pit_manager_.register_car(a.car_id);
    car_lap_count_[a.car_id] = 0;
    car_fuel_consumption_[a.car_id] = 0.0;
  }

  validate_setup();
  initialized_ = true;
  return is_valid();
}

void RaceManager::start_race() {
  if (!initialized_) return;
  session_state_ = p0::race::RaceSessionState::FORMATION;
  current_lap_ = 0;
  race_time_ = 0.0;
  session_start_time_ = 0.0;
}

void RaceManager::update(double timestamp,
                         const std::unordered_map<int, Vec2>& car_positions,
                         const std::unordered_map<int, double>& car_speeds,
                         const std::unordered_map<int, double>& car_distances,
                         const std::unordered_map<int, double>& car_fuel,
                         const std::unordered_map<int, p0::race::TireCompound>& car_tires) {
  if (!initialized_) return;

  race_time_ = timestamp;
  update_lap_counters(car_distances);
  update_session_state(timestamp);

  pit_manager_.update(timestamp, car_distances, car_speeds);

  if (session_state_ == p0::race::RaceSessionState::GREEN_FLAG_RUNNING) {
    check_fuel_strategy(timestamp);
  }
}

void RaceManager::update_session_state(double timestamp) {
  switch (session_state_) {
    case p0::race::RaceSessionState::FORMATION:
      if (timestamp - session_start_time_ > race_.formation_lap * 90.0) {
        session_state_ = p0::race::RaceSessionState::GREEN_FLAG;
      }
      break;

    case p0::race::RaceSessionState::GREEN_FLAG:
      session_state_ = p0::race::RaceSessionState::GREEN_FLAG_RUNNING;
      session_start_time_ = timestamp;
      current_lap_ = 1;
      break;

    case p0::race::RaceSessionState::GREEN_FLAG_RUNNING:
      if (current_lap_ >= race_.laps && race_.uses_laps()) {
        session_state_ = p0::race::RaceSessionState::CHECKERED_FLAG;
      }
      break;

    default:
      break;
  }
}

void RaceManager::update_lap_counters(const std::unordered_map<int, double>& car_distances) {
  if (track_.checkpoints.empty()) return;

  for (const auto& [car_id, dist] : car_distances) {
    int lap = car_lap_count_[car_id];
    double prev_dist = car_last_lap_pos_[car_id];

    if (prev_dist > 0.0 && dist < prev_dist - track_.length_m * 0.5) {
      car_lap_count_[car_id] = lap + 1;
      if (car_id == assignments_[0].car_id) {
        current_lap_ = std::max(current_lap_, car_lap_count_[car_id]);
      }
    }
    car_last_lap_pos_[car_id] = dist;
  }
}

void RaceManager::check_fuel_strategy(double) {
  for (const auto& a : assignments_) {
    double fuel_l = 0.0;
    auto it = pit_manager_.pit_system().car_state(a.car_id);
    // Strategy decision point: driver/AI uses data model to decide
    // This is where a strategy module would call request_pit_stop
  }
}

bool RaceManager::request_pit_stop(int car_id,
                                   p0::race::TireCompound new_tire,
                                   bool refuel,
                                   bool change_tires,
                                   bool repair) {
  if (session_state_ != p0::race::RaceSessionState::GREEN_FLAG_RUNNING &&
      session_state_ != p0::race::RaceSessionState::SAFETY_CAR) {
    return false;
  }

  int box_id = -1;
  for (const auto& a : assignments_) {
    if (a.car_id == car_id) {
      box_id = a.pit_box_id;
      break;
    }
  }

  if (box_id >= 0 && !pit_manager_.pit_system().is_box_free(box_id)) {
    return false;
  }

  return pit_manager_.request_pit_stop(car_id, new_tire, refuel, change_tires, repair);
}

p0::race::PitStopState RaceManager::car_pit_state(int car_id) const {
  return pit_manager_.car_state(car_id);
}

bool RaceManager::car_has_pit_violation(int car_id) const {
  const CarPitState& state = pit_manager_.pit_system().car_state(car_id);
  static CarPitState empty;
  if (&state == &empty) return false;
  return !state.violations.empty();
}

std::vector<SpeedViolation> RaceManager::pop_pending_violations() {
  return pit_manager_.pop_served_violations();
}

void RaceManager::validate_setup() {
  ValidationEngine engine(track_, race_, assignments_);
  validation_issues_ = engine.validate_all();
}

bool RaceManager::is_valid() const {
  for (const auto& issue : validation_issues_) {
    if (issue.severity == p0::race::ValidationSeverity::Error) return false;
  }
  return true;
}

std::string RaceManager::debug_report() const {
  std::ostringstream oss;
  oss << "=== Race Manager Report ===\n";
  oss << "Session: " << static_cast<int>(session_state_) << "\n";
  oss << "Lap: " << current_lap_ << "/" << race_.laps << "\n";
  oss << "Time: " << race_time_ << "s\n";
  oss << "Active pit stops: " << pit_manager_.active_pit_stops() << "\n";
  oss << "Validation issues: " << validation_issues_.size() << "\n";

  for (const auto& a : assignments_) {
    int laps = 0;
    auto lc = car_lap_count_.find(a.car_id);
    if (lc != car_lap_count_.end()) laps = lc->second;
    oss << "  Car " << a.car_id
        << " pit_state=" << static_cast<int>(pit_manager_.car_state(a.car_id))
        << " laps=" << laps << "\n";
  }

  return oss.str();
}

}
