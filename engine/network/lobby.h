#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include "input/input.h"
#include "network/network_protocol.h"
#include "network/network_session.h"
#include "ai/ai_driver.h"
#include "simulation/simulation_world.h"
#include "track/track.h"
#include "track/race_config.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace p0::network {

using p0::ai::AIDifficulty;

struct LobbyConfig {
  int max_players = kMaxPlayers;
  int track_type = 0;
  int lap_count = 3;
  bool fill_with_ai = true;
  AIDifficulty default_ai_difficulty = AIDifficulty::MEDIUM;
};

class Lobby {
 public:
  using OnLobbyUpdate = std::function<void(const std::vector<LobbySlot>&)>;
  using OnRaceStart = std::function<void()>;
  using OnError = std::function<void(const std::string&)>;

  explicit Lobby(const LobbyConfig& config = {});
  Lobby(Lobby&&) = default;
  Lobby& operator=(Lobby&&) = default;
  ~Lobby();

  bool host_game(const std::string& host_name);
  bool join_game(const std::string& host_address);
  void leave_game();
  void start_race();
  void fill_empty_slots_with_ai();

  void update(double delta_time);

  void assign_slot(int slot_index, const PlayerInfo& info);

  bool is_host() const;
  bool is_connected() const;
  int player_count() const;
  int ai_count() const;
  int slot_count() const;
  bool race_started() const { return race_started_; }
  const LobbyConfig& config() const { return config_; }
  const std::vector<LobbySlot>& slots() const { return slots_; }

  simulation::SimulationWorld& sim_world() { return sim_world_; }
  const simulation::SimulationWorld& sim_world() const { return sim_world_; }

  void set_on_lobby_update(OnLobbyUpdate cb) { on_lobby_update_ = cb; }
  void set_on_race_start(OnRaceStart cb) { on_race_start_ = cb; }
  void set_on_error(OnError cb) { on_error_ = cb; }

 private:
  LobbyConfig config_;
  std::vector<LobbySlot> slots_;
   std::unique_ptr<NetworkSession> session_;
   std::unordered_map<int, std::unique_ptr<ai::AIDriver>> ai_drivers_;
   simulation::SimulationWorld sim_world_;
  const track::Track* track_ = nullptr;
  bool race_started_ = false;

  OnLobbyUpdate on_lobby_update_;
  OnRaceStart on_race_start_;
  OnError on_error_;

  void handle_ai_inputs(double delta_time);
};

}
