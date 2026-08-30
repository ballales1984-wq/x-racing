#include "game/multiplayer_gameplay.h"
#include "vehicle/car_model.h"
#include "track/track.h"
#include "input/platform/windows_input.h"
#include "tracking/tracking_system.h"
#include "tracking/physics_trajectory.h"
#include "tracking/coordinate_converter.h"
#include "tracking/simulated_gps.h"
#include <algorithm>
#include <cmath>

namespace p0::gameplay {

MultiplayerGameplay::MultiplayerGameplay() = default;

MultiplayerGameplay::~MultiplayerGameplay() {
  shutdown();
}

bool MultiplayerGameplay::initialize(const MultiplayerConfig& config) {
  config_ = config;

  network::LobbyConfig lobby_config;
  lobby_config.max_players = config_.max_players;
  lobby_config.track_type = config_.is_host ? 0 : -1;
  lobby_config.lap_count = config_.lap_count;
  lobby_config.fill_with_ai = config_.fill_with_ai;
  lobby_config.default_ai_difficulty = config_.ai_difficulty;

  lobby_ = std::move(network::Lobby(lobby_config));

  vehicle::CarModel model = vehicle::CarRegistry::instance().all().empty()
                                ? vehicle::CarModel{}
                                : vehicle::CarRegistry::instance().all()[0];
  track::Track track(track::TrackType::Default);
  world_.initialize(track);

  tracking_converter_ = std::make_unique<p0::tracking::CoordinateConverter>(
      p0::tracking::GeographicOrigin{45.0, 11.0, 0.0});

  local_trajectory_ = std::make_unique<p0::tracking::PhysicsTrajectory>(
      *tracking_converter_, nullptr);

  auto gps = std::make_unique<p0::tracking::SimulatedGPS>(std::move(local_trajectory_));
  auto tracking = std::make_unique<p0::tracking::TrackingSystem>(std::move(gps));

  auto mapper = std::make_unique<p0::tracking::TrackMapper>(
      track, p0::tracking::TrackMapper::LocalOrigin{45.0, 11.0});
  tracking->set_mapper(std::move(mapper));

  world_.set_tracking_system(std::move(tracking));

  local_input_ = std::make_unique<input::WindowsInputManager>();

  return true;
}

void MultiplayerGameplay::shutdown() {
  leave_race();
  world_.shutdown();
  local_input_.reset();
  telemetry_.reset();
}

void MultiplayerGameplay::update(double delta_time) {
  switch (state_) {
    case MultiplayerState::MENU: update_menu(delta_time); break;
    case MultiplayerState::LOBBY: update_lobby(delta_time); break;
    case MultiplayerState::COUNTDOWN: update_countdown(delta_time); break;
    case MultiplayerState::RACING: update_racing(delta_time); break;
    case MultiplayerState::PAUSED: break;
    case MultiplayerState::RESULTS: update_results(delta_time); break;
  }
}

void MultiplayerGameplay::handle_input(const input::InputState& input) {
  if (state_ != MultiplayerState::RACING) return;

  if (config_.is_host) {
    world_.set_input(config_.local_car_id, input);
  } else if (lobby_.is_connected()) {
    lobby_.sim_world().set_input(config_.local_car_id, input);
  }
}

void MultiplayerGameplay::host_race(const std::string& host_name) {
  config_.is_host = true;
  if (lobby_.host_game(host_name)) {
    set_state(MultiplayerState::LOBBY);
  }
}

void MultiplayerGameplay::join_race(const std::string& host_address) {
  config_.is_host = false;
  if (lobby_.join_game(host_address)) {
    set_state(MultiplayerState::LOBBY);
  }
}

void MultiplayerGameplay::leave_race() {
  lobby_.leave_game();
  set_state(MultiplayerState::MENU);
  results_ = MultiplayerResults{};
}

void MultiplayerGameplay::start_countdown() {
  if (state_ != MultiplayerState::LOBBY) return;

  vehicle::CarModel model = vehicle::CarRegistry::instance().all().empty()
                                ? vehicle::CarModel{}
                                : vehicle::CarRegistry::instance().all()[0];

  if (config_.is_host) {
    std::unordered_map<int, vehicle::VehicleState> initial_states;
    for (const auto& slot : lobby_.slots()) {
      if (slot.occupied) {
        int car_id = slot.player.car_id;
        vehicle::VehicleState state;
        state.position = world_.track().get_start_position();
        state.heading = world_.track().get_start_heading();
        state.distance_along_track = static_cast<double>(slot.grid_slot) * 8.0;
        initial_states[car_id] = state;
      }
    }
    world_.reset_all(initial_states);

    if (auto* ts = world_.tracking_system()) {
      if (auto* gps = dynamic_cast<p0::tracking::SimulatedGPS*>(ts->provider())) {
        if (auto* traj = dynamic_cast<p0::tracking::PhysicsTrajectory*>(gps->trajectory())) {
          const auto* state = world_.get_state(config_.local_car_id);
          if (state) traj->set_state(state);
        }
      }
      ts->start();
    }
  }

  set_state(MultiplayerState::COUNTDOWN);
  countdown_start_time_ = 0.0;
  countdown_finished_ = false;
}

void MultiplayerGameplay::update_menu(double delta_time) {
  (void)delta_time;
}

void MultiplayerGameplay::update_lobby(double delta_time) {
  lobby_.update(delta_time);
}

void MultiplayerGameplay::update_countdown(double delta_time) {
  countdown_start_time_ += delta_time;
  if (countdown_start_time_ >= countdown_duration_ && !countdown_finished_) {
    countdown_finished_ = true;
    set_state(MultiplayerState::RACING);
  }
}

void MultiplayerGameplay::update_racing(double delta_time) {
  if (config_.is_host) {
    world_.update(delta_time, countdown_start_time_);
    check_local_finish();
  }

  if (local_input_) {
    input::InputState input = local_input_->poll();
    handle_input(input);
  }
}

void MultiplayerGameplay::update_results(double delta_time) {
  (void)delta_time;
}

void MultiplayerGameplay::check_local_finish() {
  const auto* car = world_.get_car(config_.local_car_id);
  if (!car || results_.completed) return;

  const auto& state = car->simulation->state();
  if (state.lap >= config_.lap_count && state.lap > 0) {
    results_.completed = true;
    results_.total_time = car->finish_time;
    results_.completed_laps = state.lap;
    set_state(MultiplayerState::RESULTS);

    if (on_results_) on_results_(results_);
  }
}

const vehicle::VehicleState* MultiplayerGameplay::local_state() const {
  if (config_.is_host) {
    const auto* car = world_.get_car(config_.local_car_id);
    if (car) return &car->simulation->state();
  } else {
    return world_.get_state(config_.local_car_id);
  }
  return nullptr;
}

std::string MultiplayerGameplay::race_status_text() const {
  switch (state_) {
    case MultiplayerState::MENU: return "Menu";
    case MultiplayerState::LOBBY: return "Lobby";
    case MultiplayerState::COUNTDOWN: {
      double remaining = countdown_duration_ - countdown_start_time_;
      int secs = static_cast<int>(std::ceil(std::max(0.0, remaining)));
      return "Starting in " + std::to_string(secs);
    }
    case MultiplayerState::RACING: {
      const auto* car = world_.get_car(config_.local_car_id);
      if (car) {
        const auto& state = car->simulation->state();
        return "Lap " + std::to_string(state.lap + 1) + "/" + std::to_string(config_.lap_count) +
               " | Speed: " + std::to_string(static_cast<int>(state.speed * 3.6)) + " km/h";
      }
      return "Racing";
    }
    case MultiplayerState::PAUSED: return "Paused";
    case MultiplayerState::RESULTS:
      return results_.completed ? "Finished!" : "Results";
  }
  return "";
}

void MultiplayerGameplay::set_state(MultiplayerState new_state) {
  if (state_ != new_state) {
    state_ = new_state;
    if (on_state_change_) on_state_change_(new_state);
  }
}

void MultiplayerGameplay::build_telemetry() {
  telemetry_ = std::make_unique<telemetry::Telemetry>();
  telemetry_->clear();
}

}
