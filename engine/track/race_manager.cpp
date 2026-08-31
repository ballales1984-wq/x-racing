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
    car_lap_systems_[a.car_id] = std::make_unique<p0::tracking::LapSystem>(track_.length_m, race_.laps);
    car_lap_valid_[a.car_id] = true;
    car_last_distance_[a.car_id] = 0.0;
    car_off_track_[a.car_id] = false;
    car_off_track_start_[a.car_id] = 0.0;
    car_track_limits_strikes_[a.car_id] = 0;
    car_penalty_served_[a.car_id] = false;
  }

  checkpoint_system_.set_track_length(track_.length_m);
  std::vector<p0::tracking::Checkpoint> checkpoints;
  for (const auto& cp : track_.checkpoints) {
    p0::tracking::Checkpoint tcp;
    tcp.s = cp.distance;
    tcp.tolerance = cp.width;
    tcp.id = cp.id;
    checkpoints.push_back(tcp);
  }
  checkpoint_system_.set_checkpoints(checkpoints);

  standings_tracker_.set_grid(assignments_, track_.grid.slots);

  validate_setup();
  initialized_ = true;
  return is_valid();
}

void RaceManager::start_race() {
  if (!initialized_) return;
  session_state_ = RaceSessionState::GRID;
  current_lap_ = 0;
  race_time_ = 0.0;
  session_start_time_ = 0.0;
  countdown_active_ = false;
  flag_state_ = p0::race::FlagState::GREEN;
  standings_tracker_.clear_dynamic_data();
  car_finished_times_.clear();
  emit_event(p0::race::RaceEventType::FLAG_CHANGED, 0, "GREEN");
}

void RaceManager::start_countdown() {
  if (session_state_ != RaceSessionState::GRID) return;
  countdown_active_ = true;
  countdown_start_time_ = race_time_;
}

void RaceManager::update(double timestamp,
                         const std::unordered_map<int, Vec2>& car_positions,
                         const std::unordered_map<int, double>& car_speeds,
                         const std::unordered_map<int, double>& car_distances,
                         const std::unordered_map<int, double>& car_fuel,
                         const std::unordered_map<int, p0::race::TireCompound>& car_tires) {
  if (!initialized_) return;

  race_time_ = timestamp;

  for (const auto& [car_id, dist] : car_distances) {
    car_last_distance_[car_id] = dist;
  }

  update_countdown(timestamp);
  update_lap_systems(timestamp);
  update_track_limits(timestamp, car_positions);
  update_jump_start(timestamp, car_speeds);
  update_penalty_serving();
  update_lap_counters();
  update_standings();
  update_session_state(timestamp);

  pit_manager_.update(timestamp, car_distances, car_speeds);

  if (session_state_ == RaceSessionState::GREEN_FLAG_RUNNING) {
    check_fuel_strategy(timestamp);
  }
}

void RaceManager::update_countdown(double timestamp) {
  if (session_state_ != RaceSessionState::GRID || !countdown_active_) return;

  double elapsed = timestamp - countdown_start_time_;
  double remaining = race_.countdown_duration_s - elapsed;

  if (remaining > 0.0) {
    emit_event(p0::race::RaceEventType::COUNTDOWN_TICK, 0,
               "T-" + std::to_string(static_cast<int>(remaining)));
  } else {
    session_state_ = RaceSessionState::FORMATION;
    countdown_active_ = false;
    session_start_time_ = timestamp;
    emit_event(p0::race::RaceEventType::COUNTDOWN_GO);
    emit_event(p0::race::RaceEventType::FORMATION_LAP_START);
  }
}

void RaceManager::update_lap_systems(double timestamp) {
  for (auto& [car_id, lap_system] : car_lap_systems_) {
    auto it = car_last_distance_.find(car_id);
    if (it == car_last_distance_.end()) continue;

    p0::tracking::TrackPosition pos;
    pos.s = it->second;
    pos.on_track = true;
    pos.lateral = 0.0;

    lap_system->update(pos, timestamp);

    auto events = lap_system->pop_events();
    for (const auto& evt : events) {
      if (evt.type == p0::tracking::LapEvent::Type::kLapFinished) {
        if (checkpoint_system_.all_passed()) {
          car_lap_valid_[car_id] = true;
          LapTimeEntry entry;
          entry.lap_time = evt.lap_time;
          entry.valid = true;
          entry.lap_number = evt.lap_number;
          entry.timestamp = evt.timestamp;
          car_lap_times_[car_id].push_back(entry);
          standings_tracker_.record_lap(car_id, evt.lap_time, true);
          emit_event(p0::race::RaceEventType::LAP_FINISHED, car_id);
        } else {
          car_lap_valid_[car_id] = false;
          LapTimeEntry entry;
          entry.lap_time = evt.lap_time;
          entry.valid = false;
          entry.lap_number = evt.lap_number;
          entry.timestamp = evt.timestamp;
          car_lap_times_[car_id].push_back(entry);
          standings_tracker_.record_lap(car_id, evt.lap_time, false);
          emit_event(p0::race::RaceEventType::LAP_INVALIDATED, car_id);
        }
        checkpoint_system_.reset();
      } else if (evt.type == p0::tracking::LapEvent::Type::kLapStarted) {
        emit_event(p0::race::RaceEventType::LAP_STARTED, car_id);
      } else if (evt.type == p0::tracking::LapEvent::Type::kSectorPassed) {
        emit_event(p0::race::RaceEventType::SECTOR_PASSED, car_id);
      }
    }
  }
}

void RaceManager::update_track_limits(double timestamp, const std::unordered_map<int, Vec2>& car_positions) {
  if (session_state_ != RaceSessionState::GREEN_FLAG_RUNNING) return;

  for (const auto& a : assignments_) {
    auto it = car_positions.find(a.car_id);
    if (it == car_positions.end()) continue;

    double dist_to_center = std::abs(it->second.y());
    bool off_track = dist_to_center > (track_.width_m * 0.5);

    if (off_track) {
      if (car_off_track_start_[a.car_id] == 0.0) {
        car_off_track_start_[a.car_id] = timestamp;
      } else if (timestamp - car_off_track_start_[a.car_id] > race_.off_track_warning_time_s) {
        car_track_limits_strikes_[a.car_id]++;
        car_off_track_start_[a.car_id] = timestamp;
        emit_event(p0::race::RaceEventType::TRACK_LIMITS_DETECTED, a.car_id);

        if (car_track_limits_strikes_[a.car_id] >= race_.track_limits_strikes) {
          issue_penalty(a.car_id, p0::race::ViolationType::TRACK_LIMITS);
          car_track_limits_strikes_[a.car_id] = 0;
        }
      }
    } else {
      car_off_track_start_[a.car_id] = 0.0;
    }
    car_off_track_[a.car_id] = off_track;
  }
}

void RaceManager::update_jump_start(double timestamp, const std::unordered_map<int, double>& car_speeds) {
  if (session_state_ != RaceSessionState::GRID) return;
  if (!countdown_active_) return;

  for (const auto& [car_id, speed] : car_speeds) {
    if (speed > 1.0) {
      issue_penalty(car_id, p0::race::ViolationType::JUMP_START);
      emit_event(p0::race::RaceEventType::JUMP_START_DETECTED, car_id);
    }
  }
}

void RaceManager::update_penalty_serving() {
  for (auto& [car_id, penalties] : car_penalties_) {
    for (const auto& penalty : penalties) {
      if (penalty == p0::race::ViolationType::TRACK_LIMITS ||
          penalty == p0::race::ViolationType::JUMP_START) {
        if (!car_penalty_served_[car_id]) {
          car_penalty_served_[car_id] = true;
          emit_event(p0::race::RaceEventType::PENALTY_SERVED, car_id);
        }
      }
    }
  }
}

void RaceManager::update_lap_counters() {
  for (auto& [car_id, lap_system] : car_lap_systems_) {
    int completed = lap_system->completed_laps();
    if (completed > car_lap_count_[car_id]) {
      car_lap_count_[car_id] = completed;
    }
  }
}

void RaceManager::update_standings() {
  standings_tracker_.update(car_finished_times_, car_lap_count_);

  for (const auto& a : assignments_) {
    standings_tracker_.set_car_status(a.car_id, session_state_);
    if (car_lap_systems_[a.car_id]->finished()) {
      if (car_finished_times_.find(a.car_id) == car_finished_times_.end()) {
        car_finished_times_[a.car_id] = race_time_;
        standings_tracker_.mark_finished(a.car_id, race_time_);
        emit_event(p0::race::RaceEventType::CAR_FINISHED, a.car_id);
      }
    }
  }

  if (!assignments_.empty()) {
    int leader_car = assignments_[0].car_id;
    current_lap_ = car_lap_count_[leader_car];
  }
}

void RaceManager::update_session_state(double timestamp) {
  switch (session_state_) {
    case RaceSessionState::FORMATION:
      if (timestamp - session_start_time_ > race_.formation_lap * 90.0) {
        session_state_ = RaceSessionState::GREEN_FLAG;
      }
      break;

    case RaceSessionState::GREEN_FLAG:
      session_state_ = RaceSessionState::GREEN_FLAG_RUNNING;
      session_start_time_ = timestamp;
      current_lap_ = 1;
      flag_state_ = p0::race::FlagState::GREEN;
      emit_event(p0::race::RaceEventType::GREEN_FLAG_WAVED);
      emit_event(p0::race::RaceEventType::FLAG_CHANGED, 0, "GREEN");
      break;

    case RaceSessionState::GREEN_FLAG_RUNNING: {
      bool all_finished = true;
      for (const auto& a : assignments_) {
        if (!car_lap_systems_[a.car_id]->finished()) {
          all_finished = false;
          break;
        }
      }
      if (all_finished || (current_lap_ >= race_.laps && race_.uses_laps())) {
        session_state_ = RaceSessionState::CHECKERED_FLAG;
        flag_state_ = p0::race::FlagState::CHECKERED;
        emit_event(p0::race::RaceEventType::FLAG_CHANGED, 0, "CHECKERED");
      }
      break;
    }

    case RaceSessionState::CHECKERED_FLAG: {
      bool all_crossed = true;
      for (const auto& a : assignments_) {
        if (car_finished_times_.find(a.car_id) == car_finished_times_.end()) {
          all_crossed = false;
          break;
        }
      }
      if (all_crossed) {
        session_state_ = RaceSessionState::POST_RACE;
        emit_event(p0::race::RaceEventType::RACE_ENDED);
      }
      break;
    }

    default:
      break;
  }
}

void RaceManager::check_fuel_strategy(double) {
  for (const auto& a : assignments_) {
    double fuel_l = 0.0;
    auto it = pit_manager_.pit_system().car_state(a.car_id);
    (void)fuel_l;
    (void)it;
  }
}

bool RaceManager::request_pit_stop(int car_id,
                                   p0::race::TireCompound new_tire,
                                   bool refuel,
                                   bool change_tires,
                                   bool repair) {
  if (session_state_ != RaceSessionState::GREEN_FLAG_RUNNING &&
      session_state_ != RaceSessionState::SAFETY_CAR) {
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

void RaceManager::emit_event(p0::race::RaceEventType type, int car_id, const std::string& msg) {
  p0::race::RaceEvent evt;
  evt.type = type;
  evt.car_id = car_id;
  evt.timestamp = race_time_;
  evt.message = msg;
  events_.push_back(evt);
}

std::vector<p0::race::RaceEvent> RaceManager::drain_events() {
  std::vector<p0::race::RaceEvent> result;
  while (!events_.empty()) {
    result.push_back(events_.front());
    events_.pop_front();
  }
  return result;
}

std::vector<CarStandingsEntry> RaceManager::current_standings() const {
  return standings_tracker_.current_standings();
}

CarStandingsEntry RaceManager::car_standings(int car_id) const {
  return standings_tracker_.get_car(car_id);
}

bool RaceManager::is_lap_valid(int car_id) const {
  auto it = car_lap_valid_.find(car_id);
  if (it != car_lap_valid_.end()) return it->second;
  return true;
}

std::vector<LapTimeEntry> RaceManager::car_lap_times(int car_id) const {
  auto it = car_lap_times_.find(car_id);
  if (it != car_lap_times_.end()) return it->second;
  return {};
}

void RaceManager::issue_penalty(int car_id, p0::race::ViolationType type) {
  car_penalties_[car_id].push_back(type);
  car_penalty_served_[car_id] = false;
  emit_event(p0::race::RaceEventType::PENALTY_ISSUED, car_id);
}

bool RaceManager::car_has_penalty(int car_id) const {
  auto it = car_penalties_.find(car_id);
  if (it == car_penalties_.end()) return false;
  for (const auto& p : it->second) {
    if (!car_penalty_served_.at(car_id)) return true;
  }
  return false;
}

std::vector<p0::race::ViolationType> RaceManager::car_active_penalties(int car_id) const {
  auto it = car_penalties_.find(car_id);
  if (it != car_penalties_.end()) return it->second;
  return {};
}

std::string RaceManager::debug_report() const {
  std::ostringstream oss;
  oss << "=== Race Manager Report ===\n";
  oss << "Session: " << static_cast<int>(session_state_) << "\n";
  oss << "Lap: " << current_lap_ << "/" << race_.laps << "\n";
  oss << "Time: " << race_time_ << "s\n";
  oss << "Flag: " << static_cast<int>(flag_state_) << "\n";
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
