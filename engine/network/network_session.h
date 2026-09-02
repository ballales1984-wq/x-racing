// Project 0 — UDP network session (host/client)
// Namespace: p0::network
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

// Logical role of the local peer inside a network session.
enum class SessionRole : uint8_t {
  NONE = 0,
  HOST,
  CLIENT
};

// UDP-based multiplayer session supporting host-driven authority.
// Handles input forwarding, world-state snapshots, lobby management,
// and connection lifecycle through a set of callback hooks.
class NetworkSession {
 public:
  // Callback signatures for asynchronous session events.
  using OnSnapshotReceived = std::function<void(const WorldSnapshot&)>;
  using OnPlayerJoined = std::function<void(const PlayerInfo&)>;
  using OnPlayerLeft = std::function<void(int player_id)>;
  using OnRaceStart = std::function<void(const RaceStartPacket&)>;
  using OnLobbyUpdate = std::function<void(const std::vector<LobbySlot>&)>;
  using OnDisconnect = std::function<void()>;

  NetworkSession();
  ~NetworkSession();

   // Bind as host on the given UDP port.
   bool initialize_host(int port = kServerPort);
   // Bind as client and connect to the host at host_address.
   bool initialize_client(const std::string& host_address, int port = kServerPort);
   void disconnect();
   void shutdown();

   // Send local input to the host (client only).
   void send_input(const input::InputState& input, int car_id, uint32_t sequence, double timestamp);
   // Host broadcasts a world snapshot to all clients.
   void broadcast_snapshot(const WorldSnapshot& snapshot);
   // Host sends a snapshot to a specific client.
   void send_snapshot_to_client(int player_id, const WorldSnapshot& snapshot);
   void send_packet_to_all(const void* data, int len);

  // Pump the network layer: receive packets, process callbacks, check timeouts.
  void update(double delta_time);

  SessionRole role() const { return role_; }
  int local_car_id() const { return local_car_id_; }
  int local_player_id() const { return local_player_id_; }
  bool is_host() const { return role_ == SessionRole::HOST; }
  bool is_client() const { return role_ == SessionRole::CLIENT; }
  // True when the connection is in CONNECTED, READY, or RACING state.
  bool is_connected() const { return state_ == ConnectionState::CONNECTED || state_ == ConnectionState::READY || state_ == ConnectionState::RACING; }
  ConnectionState state() const { return state_; }

   const std::unordered_map<int, PlayerInfo>& players() const { return players_; }

   // Track seed used to synchronize track generation across peers.
   uint32_t track_seed() const { return track_seed_; }
   void set_track_seed(uint32_t seed) { track_seed_ = seed; }

  void set_on_snapshot(OnSnapshotReceived cb) { on_snapshot_ = cb; }
  void set_on_player_joined(OnPlayerJoined cb) { on_player_joined_ = cb; }
  void set_on_player_left(OnPlayerLeft cb) { on_player_left_ = cb; }
  void set_on_race_start(OnRaceStart cb) { on_race_start_ = cb; }
  void set_on_lobby_update(OnLobbyUpdate cb) { on_lobby_update_ = cb; }
  void set_on_disconnect(OnDisconnect cb) { on_disconnect_ = cb; }

  private:
   // Low-level UDP send helper.
   bool send_packet(const void* data, int len, const sockaddr_in& addr);

    // Raw UDP packet received from the network layer.
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
   uint32_t track_seed_ = 12345;

   // --- Peer state ---
    std::unordered_map<int, PlayerInfo> players_;
    std::unordered_map<int, sockaddr_in> client_addresses_;
    sockaddr_in server_address_{};
    bool has_server_address_ = false;

  // --- Callbacks ---
  OnSnapshotReceived on_snapshot_;
  OnPlayerJoined on_player_joined_;
  OnPlayerLeft on_player_left_;
  OnRaceStart on_race_start_;
  OnLobbyUpdate on_lobby_update_;
  OnDisconnect on_disconnect_;

  // --- Transport ---
  SOCKET socket_fd_ = INVALID_SOCKET;
  std::vector<Packet> pending_packets_;
  double last_timeout_check_ = 0.0;
};

}
