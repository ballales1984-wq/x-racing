#include "network/lobby.h"
#include "vehicle/car_model.h"
#include "track/track.h"
#include <algorithm>
#include <cstring>

namespace p0::network {

Lobby::Lobby(const LobbyConfig& config) : config_(config) {
  slots_.resize(config_.max_players);
  for (int i = 0; i < config_.max_players; ++i) {
    slots_[i].grid_slot = i;
    slots_[i].occupied = false;
  }
}

Lobby::~Lobby() {
  leave_game();
}

bool Lobby::host_game(const std::string& host_name) {
  session_ = std::make_unique<NetworkSession>();
  if (!session_->initialize_host()) {
    if (on_error_) on_error_("Failed to host: could not bind port");
    return false;
  }

  slots_[0].player.player_id = 0;
  slots_[0].player.car_id = 0;
  slots_[0].player.type = PlayerType::HUMAN;
  slots_[0].player.state = ConnectionState::READY;
  std::strncpy(slots_[0].player.name, host_name.c_str(), kMaxPlayerNameLength - 1);
  slots_[0].occupied = true;

  session_->set_on_player_joined([this](const PlayerInfo& info) {
    int slot = info.car_id;
    if (slot >= 0 && slot < static_cast<int>(slots_.size())) {
      slots_[slot].player = info;
      slots_[slot].occupied = true;
    }
    if (on_lobby_update_) on_lobby_update_(slots_);
  });

  session_->set_on_player_left([this](int pid) {
    for (auto& slot : slots_) {
      if (slot.player.player_id == pid) {
        slot.player = PlayerInfo{};
        slot.occupied = false;
      }
    }
    if (on_lobby_update_) on_lobby_update_(slots_);
  });

  session_->set_on_disconnect([this]() {
    leave_game();
  });

  if (on_lobby_update_) on_lobby_update_(slots_);
  return true;
}

bool Lobby::join_game(const std::string& host_address) {
  session_ = std::make_unique<NetworkSession>();
  if (!session_->initialize_client(host_address)) {
    if (on_error_) on_error_("Failed to connect to host");
    return false;
  }

  if (on_lobby_update_) on_lobby_update_(slots_);
  return true;
}

void Lobby::leave_game() {
  session_.reset();
  slots_.clear();
  slots_.resize(config_.max_players);
  for (int i = 0; i < config_.max_players; ++i) {
    slots_[i].grid_slot = i;
    slots_[i].occupied = false;
  }
  race_started_ = false;
}

void Lobby::fill_empty_slots_with_ai() {
  if (!config_.fill_with_ai) return;

  vehicle::CarModel model = vehicle::CarRegistry::instance().all().empty()
                                ? vehicle::CarModel{}
                                : vehicle::CarRegistry::instance().all()[0];
  for (int i = 0; i < static_cast<int>(slots_.size()); ++i) {
    if (!slots_[i].occupied) {
      slots_[i].player.type = PlayerType::AI;
      slots_[i].player.car_id = i;
      slots_[i].player.state = ConnectionState::READY;
      std::snprintf(slots_[i].player.name, kMaxPlayerNameLength, "AI %d", i);

      sim_world_.add_car(model.params, simulation::DriverType::AI, -1,
                         std::string(slots_[i].player.name));

      slots_[i].occupied = true;
    }
  }

  if (on_lobby_update_) on_lobby_update_(slots_);
}

void Lobby::start_race() {
  if (!is_host()) return;

  fill_empty_slots_with_ai();
  race_started_ = true;

  if (on_race_start_) on_race_start_();

  RaceStartPacket start;
  start.timestamp = 0.0;
  start.countdown_duration = 3.0;
  start.track_seed = 12345;

  uint8_t buffer[sizeof(RaceStartPacket) + 1];
  buffer[0] = static_cast<uint8_t>(PacketType::RACE_START);
  std::memcpy(buffer + 1, &start, sizeof(start));

  session_->broadcast_snapshot(WorldSnapshot{});
}

void Lobby::update(double delta_time) {
  if (!session_) return;

  session_->update(delta_time);

  if (race_started_ && is_host()) {
    handle_ai_inputs(delta_time);
  }
}

void Lobby::handle_ai_inputs(double delta_time) {
  if (!ai_driver_) {
    ai_driver_ = std::make_unique<ai::AIDriver>();
    if (track_) ai_driver_->set_track(*track_);
  }

  for (auto& [car_id, car] : sim_world_.cars()) {
    if (car.driver_type == simulation::DriverType::AI) {
      const auto& state = car.simulation->state();
      ai_driver_->update(state, delta_time);
      input::InputState ai_input = ai_driver_->poll();
      sim_world_.set_ai_input(car_id, ai_input);
    }
  }
}

bool Lobby::is_host() const {
  return session_ && session_->is_host();
}

bool Lobby::is_connected() const {
  return session_ && session_->is_connected();
}

int Lobby::player_count() const {
  int count = 0;
  for (const auto& slot : slots_) {
    if (slot.occupied && slot.player.type == PlayerType::HUMAN) ++count;
  }
  return count;
}

int Lobby::ai_count() const {
  int count = 0;
  for (const auto& slot : slots_) {
    if (slot.occupied && slot.player.type == PlayerType::AI) ++count;
  }
  return count;
}

int Lobby::slot_count() const {
  return static_cast<int>(slots_.size());
}

void Lobby::assign_slot(int slot_index, const PlayerInfo& info) {
  if (slot_index >= 0 && slot_index < static_cast<int>(slots_.size())) {
    slots_[slot_index].player = info;
    slots_[slot_index].occupied = true;
  }
}

}
