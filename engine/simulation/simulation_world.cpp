#include "simulation/simulation_world.h"
#include <algorithm>

namespace p0::simulation {

SimulationWorld::SimulationWorld() = default;

SimulationWorld::~SimulationWorld() {
  shutdown();
}

bool SimulationWorld::initialize(const track::Track& track) {
  set_track(track);
  return true;
}

void SimulationWorld::shutdown() {
  cars_.clear();
  track_ = nullptr;
  next_car_id_ = 0;
  total_race_time_ = 0.0;
  race_started_ = false;
}

void SimulationWorld::set_track(const track::Track& track) {
  track_ = &track;
  for (auto& [id, car] : cars_) {
    if (car.simulation) car.simulation->set_track(track);
  }
}

int SimulationWorld::add_car(const vehicle::VehicleParams& params, DriverType driver_type, int player_id, const std::string& name) {
  int car_id = next_car_id_++;

  CarInstance car;
  car.car_id = car_id;
  car.driver_type = driver_type;
  car.player_id = player_id;
  car.name = name;

  car.simulation = std::make_unique<Simulation>();
  if (track_) car.simulation->set_track(*track_);
  car.simulation->mutable_params() = params;

  vehicle::VehicleState initial;
  if (track_) {
    initial.position = track_->get_start_position();
    initial.heading = track_->get_start_heading();
    initial.distance_along_track = 0.0;
  }
  car.simulation->reset(initial);
  car.last_state = initial;
  car.lap = 0;
  car.prev_lap = 0;
  car.lap_start_time = 0.0;

  cars_[car_id] = std::move(car);

  if (local_car_id_ == 0 && driver_type == DriverType::HUMAN) {
    local_car_id_ = car_id;
  }

  return car_id;
}

void SimulationWorld::remove_car(int car_id) {
  cars_.erase(car_id);
}

void SimulationWorld::reset_car(int car_id, const vehicle::VehicleState& initial_state) {
  auto it = cars_.find(car_id);
  if (it != cars_.end() && it->second.simulation) {
    it->second.simulation->reset(initial_state);
    it->second.last_state = initial_state;
    it->second.lap = 0;
    it->second.prev_lap = 0;
    it->second.distance_along_track = 0.0;
    it->second.finished = false;
    it->second.finish_time = 0.0;
    it->second.in_pit = false;
    it->second.lap_start_time = 0.0;
  }
}

void SimulationWorld::reset_all(const std::unordered_map<int, vehicle::VehicleState>& initial_states) {
  for (const auto& [car_id, state] : initial_states) {
    reset_car(car_id, state);
  }
}

void SimulationWorld::set_input(int car_id, const input::InputState& input) {
  auto it = cars_.find(car_id);
  if (it != cars_.end()) {
    it->second.pending_input = input;
    it->second.has_input = true;
  }
}

void SimulationWorld::set_ai_input(int car_id, const input::InputState& ai_input) {
  auto it = cars_.find(car_id);
  if (it != cars_.end() && it->second.driver_type == DriverType::AI) {
    it->second.pending_input = ai_input;
    it->second.has_input = true;
  }
}

void SimulationWorld::check_lap_completions() {
  for (auto& [car_id, car] : cars_) {
    if (car.finished) continue;

    const auto& state = car.simulation->state();
    if (state.lap > car.prev_lap) {
      int completed_lap = state.lap;
      double lap_time = total_race_time_ - car.lap_start_time;

      if (completed_lap >= 1) {
        if (on_lap_completed_) {
          on_lap_completed_(car_id, completed_lap, lap_time > 0.0 ? lap_time : 0.0);
        }
      }

      car.prev_lap = state.lap;
      car.lap = state.lap;
      car.lap_start_time = total_race_time_;
    }
  }
}

void SimulationWorld::check_race_finish() {
  for (auto& [car_id, car] : cars_) {
    if (car.finished) continue;

    const auto& state = car.simulation->state();
    if (total_laps_ > 0 && state.lap >= total_laps_) {
      car.finished = true;
      car.finish_time = total_race_time_;

      if (on_race_finished_) {
        on_race_finished_(car_id, car.finish_time, state.lap);
      }
    }
  }
}

WorldUpdateResult SimulationWorld::update(double delta_time, double timestamp) {
  WorldUpdateResult result;

  for (auto& [car_id, car] : cars_) {
    if (!car.simulation) continue;

    if (car.driver_type == DriverType::AI && !car.has_input) {
      car.pending_input = input::InputState{};
    }

    if (!car.has_input) {
      car.pending_input = input::InputState{};
    }

    car.simulation->step(car.pending_input);
    car.last_state = car.simulation->state();
    car.has_input = false;

    car.distance_along_track = car.last_state.distance_along_track;
    car.lap = car.last_state.lap;
  }

  total_race_time_ += delta_time;

  check_lap_completions();

  int leader = -1;
  double best_progress = -1.0;
  for (const auto& [car_id, car] : cars_) {
    double progress = car.last_state.distance_along_track + car.last_state.lap * (track_ ? track_->length() : 1000.0);
    if (progress > best_progress) {
      best_progress = progress;
      leader = car_id;
    }
  }

  result.leader_car_id = leader;
  result.race_time = total_race_time_;
  result.session_state = p0::race::RaceSessionState::GREEN_FLAG;
  result.current_lap = leader >= 0 ? cars_[leader].lap : 0;

  check_race_finish();

  return result;
}

WorldUpdateResult SimulationWorld::update_with_race_manager(
    double delta_time, double timestamp,
    std::function<void(double, const std::unordered_map<int, Vec2>&,
                       const std::unordered_map<int, double>&,
                       const std::unordered_map<int, double>&,
                       const std::unordered_map<int, race::TireCompound>&)> race_update_cb) {

  WorldUpdateResult result = update(delta_time, timestamp);

  if (race_update_cb && track_) {
    std::unordered_map<int, Vec2> positions;
    std::unordered_map<int, double> speeds;
    std::unordered_map<int, double> fuel;
    std::unordered_map<int, race::TireCompound> tires;

    for (const auto& [car_id, car] : cars_) {
      positions[car_id] = car.last_state.position;
      speeds[car_id] = car.last_state.speed;
      fuel[car_id] = 100.0;
      tires[car_id] = race::TireCompound::MEDIUM;
    }

    race_update_cb(timestamp, positions, speeds, fuel, tires);
  }

  return result;
}

const CarInstance* SimulationWorld::get_car(int car_id) const {
  auto it = cars_.find(car_id);
  return it != cars_.end() ? &it->second : nullptr;
}

CarInstance* SimulationWorld::get_car(int car_id) {
  auto it = cars_.find(car_id);
  return it != cars_.end() ? &it->second : nullptr;
}

const vehicle::VehicleState* SimulationWorld::get_state(int car_id) const {
  auto it = cars_.find(car_id);
  if (it != cars_.end() && it->second.simulation) {
    return &it->second.simulation->state();
  }
  return nullptr;
}

const track::Track& SimulationWorld::track() const {
  assert(track_ && "SimulationWorld::track() called before set_track()");
  return *track_;
}

int SimulationWorld::active_car_count() const {
  int count = 0;
  for (const auto& [id, car] : cars_) {
    if (!car.finished) ++count;
  }
  return count;
}

}
