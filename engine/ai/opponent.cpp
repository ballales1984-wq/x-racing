#include "ai/opponent.h"
#include <algorithm>

namespace p0::ai {

//! @brief Constructs an opponent with AI driver parameters and name.
//! @param params AI driver configuration.
//! @param name Display name for the opponent.
Opponent::Opponent(const AIDriverParams& params, const std::string& name)
    : driver_(params), state_() {
  state_.name = name;
  state_.car_id = -1;
}

//! @brief Sets the track reference for the AI driver.
//! @param track The track data.
void Opponent::set_track(const track::Track& track) {
  driver_.set_track(track);
}

//! @brief Sets the racing line for the AI driver to follow.
//! @param samples Vector of racing line samples.
void Opponent::set_racing_line(const std::vector<track::RacingLineSample>& samples) {
  driver_.set_racing_line(samples);
}

//! @brief Resets the opponent state for a new race.
//! @param state Initial vehicle state.
void Opponent::reset(const vehicle::VehicleState& state) {
  state_.distance_along_track = state.distance_along_track;
  state_.speed_m_s = state.speed;
  state_.lap = state.lap;
  state_.finished = false;
  state_.total_time = 0.0;
  state_.best_lap_time = 0.0;
}

//! @brief Updates the opponent's AI and returns the input state.
//! @param state Current vehicle state.
//! @param delta_time Time elapsed since last update.
//! @return InputState with throttle, brake, and steering.
input::InputState Opponent::update(const vehicle::VehicleState& state, double delta_time) {
  state_.distance_along_track = state.distance_along_track;
  state_.speed_m_s = state.speed;
  state_.lap = state.lap;

  driver_.update(state, delta_time);
  return driver_.poll();
}

// ---------------------------------------------------------------------------
// OpponentManager implementation
// ---------------------------------------------------------------------------

//! @brief Constructs the opponent manager.
//! @param track Pointer to the track data.
OpponentManager::OpponentManager(const track::Track* track) : track_(track) {}

//! @brief Sets the track for all managed opponents.
//! @param track The track data.
void OpponentManager::set_track(const track::Track& track) {
  track_ = &track;
  for (auto& opp : opponents_) {
    opp.set_track(track);
  }
}

//! @brief Sets the racing line for all managed opponents.
//! @param samples Vector of racing line samples.
void OpponentManager::set_racing_line(const std::vector<track::RacingLineSample>& samples) {
  racing_line_ = samples;
  for (auto& opp : opponents_) {
    opp.set_racing_line(samples);
  }
}

//! @brief Adds a new opponent to the manager.
//! @param params AI driver configuration.
//! @param name Display name for the opponent.
//! @return The assigned car ID.
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

//! @brief Removes an opponent by car ID.
//! @param car_id The car ID to remove.
void OpponentManager::remove_opponent(int car_id) {
  auto it = std::find_if(opponents_.begin(), opponents_.end(),
    [car_id](const Opponent& o) { return o.state().car_id == car_id; });
  if (it != opponents_.end()) {
    opponents_.erase(it);
  }
}

//! @brief Resets all opponents to initial states.
//! @param states Vector of initial vehicle states.
void OpponentManager::reset_all(const std::vector<vehicle::VehicleState>& states) {
  for (size_t i = 0; i < opponents_.size() && i < states.size(); ++i) {
    opponents_[i].reset(states[i]);
  }
}

//! @brief Updates all opponents and returns their input states.
//!        Builds a "nearby cars" list for each opponent for traffic-aware AI.
//! @param states Vector of current vehicle states.
//! @param delta_time Time elapsed since last update.
//! @return Vector of InputState for each opponent.
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

//! @brief Returns a pointer to an opponent by car ID (const).
//! @param car_id The car ID to find.
//! @return Pointer to the opponent, or nullptr if not found.
const Opponent* OpponentManager::get(int car_id) const {
  auto it = std::find_if(opponents_.begin(), opponents_.end(),
    [car_id](const Opponent& o) { return o.state().car_id == car_id; });
  return it != opponents_.end() ? &(*it) : nullptr;
}

//! @brief Returns a pointer to an opponent by car ID (mutable).
//! @param car_id The car ID to find.
//! @return Pointer to the opponent, or nullptr if not found.
Opponent* OpponentManager::get(int car_id) {
  auto it = std::find_if(opponents_.begin(), opponents_.end(),
    [car_id](const Opponent& o) { return o.state().car_id == car_id; });
  return it != opponents_.end() ? &(*it) : nullptr;
}

//! @brief Returns a vector of all opponent pointers (const).
//! @return Vector of const pointers to all opponents.
std::vector<const Opponent*> OpponentManager::opponents() const {
  std::vector<const Opponent*> result;
  result.reserve(opponents_.size());
  for (const auto& opp : opponents_) {
    result.push_back(&opp);
  }
  return result;
}

//! @brief Clears all opponents and resets the ID counter.
void OpponentManager::clear() {
  opponents_.clear();
  next_id_ = 1;
}

}