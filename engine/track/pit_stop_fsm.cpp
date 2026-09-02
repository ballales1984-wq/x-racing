#include "pit_stop_fsm.h"
#include "pit_service_unit.h"
#include <algorithm>

namespace p0::track {

//! @brief Constructs the pit stop FSM for a single car.
//! @param car_id The car this FSM controls.
//! @param pit_system Reference to the pit lane system.
//! @param psu_manager Optional pointer to the pit service unit manager.
PitStopFSM::PitStopFSM(int car_id, const PitLaneSystem& pit_system, PitServiceUnitManager* psu_manager)
    : car_id_(car_id), pit_system_(&pit_system), psu_manager_(psu_manager), state_(p0::race::PitStopState::NONE) {}

//! @brief Requests a pit stop. Only valid when in NONE state.
//! @param new_tire Tire compound to fit.
//! @param refuel Whether to add fuel.
//! @param tires Whether to change tires.
//! @param repair Whether to repair damage.
void PitStopFSM::request_stop(p0::race::TireCompound new_tire, bool refuel, bool tires, bool repair) {
  if (state_ == p0::race::PitStopState::NONE) {
    state_ = p0::race::PitStopState::REQUESTED;
    state_entry_time_ = 0.0;
    car_state_.state = state_;
    requested_tire_ = new_tire;
    request_refuel_ = refuel;
    request_tires_ = tires;
    request_repair_ = repair;
    service_requested_ = true;

    if (car_state_.assigned_box_id >= 0 && psu_manager_) {
      service_duration_ = psu_manager_->estimated_service_time(
          car_state_.assigned_box_id, refuel, tires, repair);
    }
  }
}

//! @brief Advances the FSM by one tick. Handles all state transitions.
//! @param timestamp Current race time in seconds.
//! @param car_track_pos_m Car's position along the track centerline.
//! @param car_speed_m_s Car's current speed.
void PitStopFSM::update(double timestamp, double car_track_pos_m, double car_speed_m_s) {
  if (state_ == p0::race::PitStopState::NONE || state_ == p0::race::PitStopState::COMPLETE ||
      state_ == p0::race::PitStopState::ABANDONED) {
    return;
  }

  const double dt = (state_entry_time_ > 0.0) ? (timestamp - state_entry_time_) : 0.0;

  switch (state_) {
    case p0::race::PitStopState::REQUESTED:
      transition_to(p0::race::PitStopState::APPROACHING_PIT_LANE, timestamp);
      break;

    case p0::race::PitStopState::APPROACHING_PIT_LANE: {
      double entry_pos = pit_system_->definition().entry.transform.position.x();
      if (car_track_pos_m >= entry_pos - 20.0) {
        transition_to(p0::race::PitStopState::ENTERING_PIT_LANE, timestamp);
      }
      break;
    }

    case p0::race::PitStopState::ENTERING_PIT_LANE: {
      car_state_.pit_entry_time = timestamp;
      if (car_speed_m_s <= pit_system_->definition().speed_zone.speed_limit_m_s) {
        transition_to(p0::race::PitStopState::PIT_LANE_NAVIGATION, timestamp);
      }
      break;
    }

    case p0::race::PitStopState::PIT_LANE_NAVIGATION: {
      if (car_state_.assigned_box_id >= 0) {
        transition_to(p0::race::PitStopState::BOX_ASSIGNED, timestamp);
      }
      break;
    }

    case p0::race::PitStopState::BOX_ASSIGNED:
      transition_to(p0::race::PitStopState::ALIGNING_BOX, timestamp);
      break;

    case p0::race::PitStopState::ALIGNING_BOX:
      check_box_alignment(timestamp, car_track_pos_m);
      break;

    case p0::race::PitStopState::STOPPED_AT_BOX:
      check_release_conditions(timestamp);
      break;

    case p0::race::PitStopState::SERVICING: {
      if (timestamp - service_started_ >= service_duration_) {
        car_state_.last_service.total_service_time_s = service_duration_;
        if (car_state_.assigned_box_id >= 0) {
          deactivate_box_psus(car_state_.assigned_box_id, timestamp);
        }
        transition_to(p0::race::PitStopState::RELEASE_AUTHORIZED, timestamp);
      }
      break;
    }

    case p0::race::PitStopState::RELEASE_AUTHORIZED:
      transition_to(p0::race::PitStopState::EXITING_BOX, timestamp);
      break;

    case p0::race::PitStopState::EXITING_BOX:
      transition_to(p0::race::PitStopState::PIT_EXIT_NAVIGATION, timestamp);
      break;

    case p0::race::PitStopState::PIT_EXIT_NAVIGATION: {
      double exit_pos = pit_system_->definition().exit.transform.position.x();
      if (car_track_pos_m >= exit_pos) {
        transition_to(p0::race::PitStopState::TRACK_REENTRY, timestamp);
      }
      break;
    }

    case p0::race::PitStopState::TRACK_REENTRY:
      transition_to(p0::race::PitStopState::COMPLETE, timestamp);
      ++car_state_.pit_stops_completed;
      break;

    default:
      break;
  }
}

//! @brief Abandons the current pit stop. Only valid if not already complete or abandoned.
void PitStopFSM::abandon() {
  if (state_ != p0::race::PitStopState::COMPLETE && state_ != p0::race::PitStopState::ABANDONED) {
    transition_to(p0::race::PitStopState::ABANDONED, 0.0);
  }
}

//! @brief Checks if the pit stop has finished (complete or abandoned).
//! @return true if the FSM is in a terminal state.
bool PitStopFSM::is_complete() const {
  return state_ == p0::race::PitStopState::COMPLETE || state_ == p0::race::PitStopState::ABANDONED;
}

//! @brief Checks if the pit stop is currently in progress.
//! @return true if the FSM is active (not NONE and not complete).
bool PitStopFSM::is_active() const {
  return state_ != p0::race::PitStopState::NONE && !is_complete();
}

//! @brief Checks if the car has any pit lane violations.
//! @return true if violations exist.
bool PitStopFSM::has_violation() const {
  return !car_state_.violations.empty();
}

//! @brief Returns a human-readable name for the current FSM state.
//! @return String representation of the state.
std::string PitStopFSM::state_name() const {
  switch (state_) {
    case p0::race::PitStopState::NONE:                  return "NONE";
    case p0::race::PitStopState::REQUESTED:             return "REQUESTED";
    case p0::race::PitStopState::APPROACHING_PIT_LANE:  return "APPROACHING_PIT_LANE";
    case p0::race::PitStopState::ENTERING_PIT_LANE:     return "ENTERING_PIT_LANE";
    case p0::race::PitStopState::PIT_LANE_NAVIGATION:   return "PIT_LANE_NAVIGATION";
    case p0::race::PitStopState::BOX_ASSIGNED:          return "BOX_ASSIGNED";
    case p0::race::PitStopState::ALIGNING_BOX:          return "ALIGNING_BOX";
    case p0::race::PitStopState::STOPPED_AT_BOX:        return "STOPPED_AT_BOX";
    case p0::race::PitStopState::SERVICING:             return "SERVICING";
    case p0::race::PitStopState::RELEASE_AUTHORIZED:    return "RELEASE_AUTHORIZED";
    case p0::race::PitStopState::EXITING_BOX:           return "EXITING_BOX";
    case p0::race::PitStopState::PIT_EXIT_NAVIGATION:   return "PIT_EXIT_NAVIGATION";
    case p0::race::PitStopState::TRACK_REENTRY:         return "TRACK_REENTRY";
    case p0::race::PitStopState::COMPLETE:              return "COMPLETE";
    case p0::race::PitStopState::ABANDONED:             return "ABANDONED";
    default:                                  return "UNKNOWN";
  }
}

//! @brief Returns a debug string with car ID, state, box, and completed stops.
//! @return Formatted debug string.
std::string PitStopFSM::debug_string() const {
  return "car=" + std::to_string(car_id_) +
         " state=" + state_name() +
         " box=" + std::to_string(car_state_.assigned_box_id) +
         " stops=" + std::to_string(car_state_.pit_stops_completed);
}

//! @brief Transitions the FSM to a new state and records the entry timestamp.
//! @param new_state The state to transition to.
//! @param timestamp Current race time in seconds.
void PitStopFSM::transition_to(p0::race::PitStopState new_state, double timestamp) {
  state_ = new_state;
  state_entry_time_ = timestamp;
  car_state_.state = new_state;
}

//! @brief Checks if the car is aligned with its assigned pit box.
//! @param timestamp Current race time in seconds.
//! @param car_track_pos_m Car's position along the track centerline.
void PitStopFSM::check_box_alignment(double timestamp, double car_track_pos_m) {
  double box_pos = 0.0;
  if (car_state_.assigned_box_id >= 0 &&
      car_state_.assigned_box_id < static_cast<int>(pit_system_->definition().boxes.size())) {
    box_pos = pit_system_->definition().boxes[car_state_.assigned_box_id].position.position.x();
  }

  double dist_to_box = std::abs(car_track_pos_m - box_pos);
  if (dist_to_box < 2.0) {
    transition_to(p0::race::PitStopState::STOPPED_AT_BOX, timestamp);
    car_state_.box_stop_time = timestamp;
  }
}

//! @brief Checks if service is complete and transitions to release or servicing state.
//! @param timestamp Current race time in seconds.
void PitStopFSM::check_release_conditions(double timestamp) {
  if (service_requested_ && service_duration_ > 0.0) {
    transition_to(p0::race::PitStopState::SERVICING, timestamp);
    service_started_ = timestamp;
    if (car_state_.assigned_box_id >= 0) {
      activate_box_psus(car_state_.assigned_box_id, car_id_, timestamp);
    }
  } else {
    transition_to(p0::race::PitStopState::RELEASE_AUTHORIZED, timestamp);
  }
}

//! @brief Activates all PSUs assigned to a pit box for service.
//! @param box_id The pit box ID.
//! @param car_id The car being serviced.
//! @param timestamp Current race time in seconds.
//! @return true if activation succeeded.
bool PitStopFSM::activate_box_psus(int box_id, int car_id, double timestamp) {
  if (!psu_manager_) return false;
  return psu_manager_->activate_units_for_service(box_id, car_id, timestamp);
}

//! @brief Deactivates all PSUs assigned to a pit box after service.
//! @param box_id The pit box ID.
//! @param timestamp Current race time in seconds.
void PitStopFSM::deactivate_box_psus(int box_id, double timestamp) {
  if (!psu_manager_) return;
  psu_manager_->deactivate_units(box_id, timestamp);
}

// ---------------------------------------------------------------------------
// PitStopManager implementation
// ---------------------------------------------------------------------------

//! @brief Constructs the pit stop manager with a pit lane definition.
//! @param pit_def The pit lane definition.
PitStopManager::PitStopManager(const PitLaneDefinition& pit_def)
    : pit_system_(pit_def) {}

//! @brief Registers a car for pit stop tracking.
//! @param car_id The car to register.
void PitStopManager::register_car(int car_id) {
  fsm_map_.emplace(car_id, PitStopFSM(car_id, pit_system_, psu_manager_));
}

//! @brief Unregisters a car and resets its pit state.
//! @param car_id The car to unregister.
void PitStopManager::unregister_car(int car_id) {
  pit_system_.reset_car(car_id);
  fsm_map_.erase(car_id);
}

//! @brief Requests a pit stop for a car with specified service options.
//! @param car_id The car requesting the pit stop.
//! @param new_tire Tire compound to fit.
//! @param refuel Whether to add fuel.
//! @param change_tires Whether to change tires.
//! @param repair Whether to repair damage.
//! @return true if the pit stop was successfully requested.
bool PitStopManager::request_pit_stop(int car_id,
                                      p0::race::TireCompound new_tire,
                                      bool refuel,
                                      bool change_tires,
                                      bool repair) {
  auto it = fsm_map_.find(car_id);
  if (it == fsm_map_.end()) return false;

  PitStopFSM& fsm = it->second;
  if (fsm.is_active()) return false;

  fsm.request_stop(new_tire, refuel, change_tires, repair);
  fsm.mutable_car_state().assigned_box_id = pit_system_.assign_box(car_id, 0);
  fsm.mutable_car_state().state = p0::race::PitStopState::REQUESTED;

  return true;
}

//! @brief Updates all active pit stop FSMs.
//! @param timestamp Current race time in seconds.
//! @param car_positions Map of car ID to track position.
//! @param car_speeds Map of car ID to current speed.
void PitStopManager::update(double timestamp,
                            const std::unordered_map<int, double>& car_positions,
                            const std::unordered_map<int, double>& car_speeds) {
  for (auto& [car_id, fsm] : fsm_map_) {
    double pos = 0.0;
    double speed = 0.0;
    auto pit = car_positions.find(car_id);
    if (pit != car_positions.end()) pos = pit->second;
    auto spd = car_speeds.find(car_id);
    if (spd != car_speeds.end()) speed = spd->second;

    fsm.update(timestamp, pos, speed);
  }
}

//! @brief Returns the current pit stop state for a car.
//! @param car_id The car to query.
//! @return Current PitStopState enum value.
p0::race::PitStopState PitStopManager::car_state(int car_id) const {
  auto it = fsm_map_.find(car_id);
  if (it == fsm_map_.end()) return p0::race::PitStopState::NONE;
  return it->second.state();
}

//! @brief Checks if a car has an active pit stop.
//! @param car_id The car to check.
//! @return true if the car's pit stop FSM is active.
bool PitStopManager::is_pit_stop_active(int car_id) const {
  auto it = fsm_map_.find(car_id);
  if (it == fsm_map_.end()) return false;
  return it->second.is_active();
}

//! @brief Returns the number of currently active pit stops.
//! @return Count of active pit stops.
int PitStopManager::active_pit_stops() const {
  int count = 0;
  for (const auto& [id, fsm] : fsm_map_) {
    if (fsm.is_active()) ++count;
  }
  return count;
}

//! @brief Retrieves and clears all pending speed violations from pit lane.
//! @return Vector of unserved SpeedViolation structs.
std::vector<SpeedViolation> PitStopManager::pop_served_violations() {
  auto v = pit_system_.process_speed_violations();
  std::copy(v.begin(), v.end(), std::back_inserter(violations_));
  std::vector<SpeedViolation> result;
  for (auto& viol : violations_) {
    if (!viol.served) result.push_back(viol);
  }
  violations_.clear();
  return result;
}

}
