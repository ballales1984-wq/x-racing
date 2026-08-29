#pragma once

#include "common.h"
#include "network/network_protocol.h"
#include "network/snapshot.h"
#include "input/input.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
#endif

namespace p0::network {

enum class SessionRole : uint8_t {
  NONE = 0,
  HOST,
  CLIENT
};

class NetworkSession {
 public:
  using OnSnapshotReceived = std::function<void(const WorldSnapshot&)>;
  using OnPlayerJoined = std::function<void(const PlayerInfo&)>;
  using OnPlayerLeft = std::function<void(int player_id)>;
  using OnRaceStart = std::function<void(const RaceStartPacket&)>;
  using OnLobbyUpdate = std::function<void(const std::vector<LobbySlot>&)>;
  using OnDisconnect = std::function<void()>;

  NetworkSession();
  ~NetworkSession();

  bool initialize_host(int port = kServerPort);
  bool initialize_client(const std::string& host_address, int port = kServerPort);
  void shutdown();

  void send_input(const input::InputState& input, int car_id, uint32_t sequence, double timestamp);
  void broadcast_snapshot(const WorldSnapshot& snapshot);
  void send_snapshot_to_client(int player_id, const WorldSnapshot& snapshot);

  void update(double delta_time);

  SessionRole role() const { return role_; }
  int local_car_id() const { return local_car_id_; }
  int local_player_id() const { return local_player_id_; }
  bool is_host() const { return role_ == SessionRole::HOST; }
  bool is_client() const { return role_ == SessionRole::CLIENT; }
  bool is_connected() const { return state_ == ConnectionState::CONNECTED || state_ == ConnectionState::READY || state_ == ConnectionState::RACING; }
  ConnectionState state() const { return state_; }

  const std::unordered_map<int, PlayerInfo>& players() const { return players_; }

  void set_on_snapshot(OnSnapshotReceived cb) { on_snapshot_ = cb; }
  void set_on_player_joined(OnPlayerJoined cb) { on_player_joined_ = cb; }
  void set_on_player_left(OnPlayerLeft cb) { on_player_left_ = cb; }
  void set_on_race_start(OnRaceStart cb) { on_race_start_ = cb; }
  void set_on_lobby_update(OnLobbyUpdate cb) { on_lobby_update_ = cb; }
  void set_on_disconnect(OnDisconnect cb) { on_disconnect_ = cb; }

  private:
   bool send_packet(const void* data, int len, const sockaddr_in& addr);

   struct Packet {
    uint8_t data[kMaxPacketSize];
    int size = 0;
    std::string from_address;
  };

  SessionRole role_ = SessionRole::NONE;
  ConnectionState state_ = ConnectionState::DISCONNECTED;
  int local_car_id_ = 0;
  int local_player_id_ = -1;
  char session_id_[16] = {};

  std::unordered_map<int, PlayerInfo> players_;

  OnSnapshotReceived on_snapshot_;
  OnPlayerJoined on_player_joined_;
  OnPlayerLeft on_player_left_;
  OnRaceStart on_race_start_;
  OnLobbyUpdate on_lobby_update_;
  OnDisconnect on_disconnect_;

  int socket_fd_ = -1;
  std::vector<Packet> pending_packets_;
  double last_timeout_check_ = 0.0;
};

}
