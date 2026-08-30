#include "ai/opponent.h"
#include <algorithm>

namespace p0::ai {

Opponent::Opponent(const AIDriverParams& params, const std::string& name)
    : driver_(params), state_() {
  state_.name = name;
  state_.car_id = -1;
}

void Opponent::set_track(const track::Track& track) {
  driver_.set_track(track);
}

void Opponent::set_racing_line(const std::vector<track::RacingLineSample>& samples) {
  driver_.set_racing_line(samples);
}

void Opponent::reset(const vehicle::VehicleState& state) {
  state_.distance_along_track = state.distance_along_track;
  state_.speed_m_s = state.speed;
  state_.lap = state.lap;
  state_.finished = false;
  state_.total_time = 0.0;
  state_.best_lap_time = 0.0;
}

input::InputState Opponent::update(const vehicle::VehicleState& state, double delta_time) {
  state_.distance_along_track = state.distance_along_track;
  state_.speed_m_s = state.speed;
  state_.lap = state.lap;

  driver_.update(state, delta_time);
  return driver_.poll();
}

OpponentManager::OpponentManager(const track::Track* track) : track_(track) {}

void OpponentManager::set_track(const track::Track& track) {
  track_ = &track;
  for (auto& opp : opponents_) {
    opp.set_track(track);
  }
}

void OpponentManager::set_racing_line(const std::vector<track::RacingLineSample>& samples) {
  racing_line_ = samples;
  for (auto& opp : opponents_) {
    opp.set_racing_line(samples);
  }
}

int OpponentManager::add_opponent(const AIDriverParams& params, const std::string& name) {
  Opponent opp(params, name);
  opp.set_track(*track_);
  if (!racing_line_.empty()) {
    opp.set_racing_line(racing_line_);
  }
  int id = next_id_++;
  OpponentState s;
  s.car_id = id;
  s.name = name;
  s.difficulty = params.difficulty;
  opp.set_state(s);
  opponents_.push_back(std::move(opp));
  return id;
}

void OpponentManager::remove_opponent(int car_id) {
  auto it = std::find_if(opponents_.begin(), opponents_.end(),
    [car_id](const Opponent& o) { return o.state().car_id == car_id; });
  if (it != opponents_.end()) {
    opponents_.erase(it);
  }
}

void OpponentManager::reset_all(const std::vector<vehicle::VehicleState>& states) {
  for (size_t i = 0; i < opponents_.size() && i < states.size(); ++i) {
    opponents_[i].reset(states[i]);
  }
}

std::vector<input::InputState> OpponentManager::update_all(
    const std::vector<vehicle::VehicleState>& states, double delta_time) {
  std::vector<input::InputState> inputs;
  inputs.reserve(opponents_.size());

  for (size_t i = 0; i < opponents_.size() && i < states.size(); ++i) {
    // Build the "nearby cars" list: every car except the current opponent.
    // This gives each AI the traffic information needed for overtaking,
    // defense, and collision avoidance.
    std::vector<vehicle::VehicleState> nearby;
    nearby.reserve(states.size() - 1);
    for (size_t j = 0; j < states.size(); ++j) {
      if (j != i) {
        nearby.push_back(states[j]);
      }
    }
    opponents_[i].driver().set_nearby_cars(nearby);
    inputs.push_back(opponents_[i].update(states[i], delta_time));
  }

  return inputs;
}

const Opponent* OpponentManager::get(int car_id) const {
  auto it = std::find_if(opponents_.begin(), opponents_.end(),
    [car_id](const Opponent& o) { return o.state().car_id == car_id; });
  return it != opponents_.end() ? &(*it) : nullptr;
}

Opponent* OpponentManager::get(int car_id) {
  auto it = std::find_if(opponents_.begin(), opponents_.end(),
    [car_id](const Opponent& o) { return o.state().car_id == car_id; });
  return it != opponents_.end() ? &(*it) : nullptr;
}

std::vector<const Opponent*> OpponentManager::opponents() const {
  std::vector<const Opponent*> result;
  result.reserve(opponents_.size());
  for (const auto& opp : opponents_) {
    result.push_back(&opp);
  }
  return result;
}

void OpponentManager::clear() {
  opponents_.clear();
  next_id_ = 1;
}

}
