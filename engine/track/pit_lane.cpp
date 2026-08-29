#include "pit_lane.h"
#include <algorithm>

namespace p0::track {

PitLaneSystem::PitLaneSystem(const PitLaneDefinition& def) : def_(def) {}

int PitLaneSystem::assign_box(int car_id, int team_id) {
  int box_id = find_box_for_team(team_id);
  if (box_id < 0) box_id = find_free_box();
  if (box_id < 0) return -1;

  CarPitState& state = car_states_[car_id];
  state.car_id = car_id;
  state.assigned_box_id = box_id;

  if (box_id >= 0 && box_id < static_cast<int>(def_.boxes.size())) {
    PitBox& box = def_.boxes[box_id];
    box.state = BoxState::OCCUPIED;
    box.assigned_car_id = car_id;
  }

  return box_id;
}

void PitLaneSystem::release_box(int car_id) {
  auto it = car_states_.find(car_id);
  if (it == car_states_.end()) return;

  int box_id = it->second.assigned_box_id;
  if (box_id >= 0 && box_id < static_cast<int>(def_.boxes.size())) {
    PitBox& box = def_.boxes[box_id];
    box.state = BoxState::FREE;
    box.assigned_car_id = -1;
    box.occupied_since = 0.0;
  }

  it->second.assigned_box_id = -1;
}

int PitLaneSystem::find_box_for_team(int team_id) const {
  for (const auto& box : def_.boxes) {
    if (box.team_id == team_id && box.state == BoxState::FREE) return box.box_id;
  }
  return -1;
}

int PitLaneSystem::find_free_box() const {
  for (const auto& box : def_.boxes) {
    if (box.state == BoxState::FREE) return box.box_id;
  }
  return -1;
}

bool PitLaneSystem::is_box_free(int box_id) const {
  if (box_id < 0 || box_id >= static_cast<int>(def_.boxes.size())) return false;
  return def_.boxes[box_id].state == BoxState::FREE;
}

void PitLaneSystem::on_cross_speed_start(int car_id, double timestamp, double track_pos) {
  CarPitState& state = car_states_[car_id];
  state.speed_zone_entry_time = timestamp;
  state.speed_zone_entry_position_m = track_pos;
  state.speed_zone_entered = true;
}

void PitLaneSystem::on_cross_speed_end(int car_id, double timestamp, double track_pos) {
  CarPitState& state = car_states_[car_id];
  if (!state.speed_zone_entered) return;

  double dist = std::abs(track_pos - state.speed_zone_entry_position_m);
  double time = timestamp - state.speed_zone_entry_time;

  if (time > kEpsilon) {
    double avg_speed = average_speed_m_s(dist, time);
    if (avg_speed > def_.speed_zone.effective_limit_m_s()) {
      SpeedViolation v;
      v.car_id = car_id;
      v.recorded_speed_m_s = avg_speed;
      v.limit_m_s = def_.speed_zone.speed_limit_m_s;
      v.timestamp = timestamp;
      v.track_position_m = track_pos;
      v.type = def_.speed_zone.violation_type;
      v.penalty = def_.speed_zone.penalty;
      violations_.push_back(v);
    }
  }

  state.speed_zone_entered = false;
}

std::vector<SpeedViolation> PitLaneSystem::process_speed_violations() {
  std::vector<SpeedViolation> result;
  for (auto& v : violations_) {
    if (!v.served) {
      result.push_back(v);
      v.served = true;
    }
  }
  return result;
}

CarPitState& PitLaneSystem::car_state(int car_id) {
  return car_states_[car_id];
}

const CarPitState& PitLaneSystem::car_state(int car_id) const {
  static CarPitState empty;
  auto it = car_states_.find(car_id);
  return (it != car_states_.end()) ? it->second : empty;
}

void PitLaneSystem::reset_car(int car_id) {
  release_box(car_id);
  car_states_.erase(car_id);
}

int PitLaneSystem::cars_in_pit_lane() const {
  int count = 0;
  for (const auto& [id, state] : car_states_) {
    if ((state.state >= p0::race::PitStopState::ENTERING_PIT_LANE &&
         state.state <= p0::race::PitStopState::PIT_EXIT_NAVIGATION)) {
      ++count;
    }
  }
  return count;
}

int PitLaneSystem::cars_stopped() const {
  int count = 0;
  for (const auto& [id, state] : car_states_) {
    if (state.state == p0::race::PitStopState::STOPPED_AT_BOX ||
        state.state == p0::race::PitStopState::SERVICING) {
      ++count;
    }
  }
  return count;
}

bool PitLaneSystem::can_enter_pit_lane() const {
  return cars_in_pit_lane() < def_.max_cars;
}

bool PitLaneSystem::can_stop_at_box() const {
  return cars_stopped() < def_.max_stopped;
}

double PitLaneSystem::average_speed_m_s(double dist, double time) const {
  if (time <= kEpsilon) return 0.0;
  return dist / time;
}

 PitServiceResult PitLaneSystem::calculate_service(
      const p0::race::CarFuelState& fuel,
      const p0::race::CarTireState& tires,
      p0::race::TireCompound new_tire,
      bool do_refuel,
      bool do_tires,
      bool do_repair,
      double damage_hp,
      double refuel_rate_l_s,
      double tire_change_time_s,
      double repair_rate_hp_s) const {

  PitServiceResult result;

  if (do_refuel) {
    double space = fuel.fuel_capacity_l - fuel.current_fuel_l;
    double add = std::min(space, fuel.fuel_capacity_l * 0.5);
    result.fuel_added_l = std::max(0.0, add);
    result.refuel_time_s = (result.fuel_added_l > 0.0 && refuel_rate_l_s > kEpsilon)
                               ? result.fuel_added_l / refuel_rate_l_s
                               : 0.0;
    result.refueled = result.fuel_added_l > kEpsilon;
  }

  if (do_tires && tires.current_compound != new_tire) {
    result.tire_change_time_s = tire_change_time_s;
    result.tires_changed = true;
    result.new_compound = new_tire;
  }

  if (do_repair && damage_hp > 0.0 && repair_rate_hp_s > kEpsilon) {
    result.repair_time_s = damage_hp / repair_rate_hp_s;
    result.repaired = true;
  }

  result.total_service_time_s = std::max(
      result.refuel_time_s,
      std::max(result.tire_change_time_s, result.repair_time_s));

  return result;
}

}
