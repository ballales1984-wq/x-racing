#include "network/network_session.h"
#include "network/snapshot.h"
#include "input/input.h"

#include <random>

#if defined(_WIN32)
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #define SOCKET int
  #define INVALID_SOCKET -1
  #define SOCKET_ERROR -1
  #define closesocket close
#endif

#include <cstring>

namespace p0::network {

bool NetworkSession::send_packet(const void* data, int len, const sockaddr_in& addr) {
  if (socket_fd_ == INVALID_SOCKET) return false;
  int sent = sendto(socket_fd_, reinterpret_cast<const char*>(data), len, 0,
                    reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
  return sent == len;
}

NetworkSession::NetworkSession() = default;

NetworkSession::~NetworkSession() {
  disconnect();
  shutdown();
}

void NetworkSession::disconnect() {
  if (socket_fd_ == INVALID_SOCKET || role_ == SessionRole::NONE) return;

  if (role_ == SessionRole::CLIENT) {
    DisconnectPacket disc;
    disc.reason = 0;
    uint8_t buffer[sizeof(DisconnectPacket) + 1];
    buffer[0] = static_cast<uint8_t>(PacketType::DISCONNECT);
    std::memcpy(buffer + 1, &disc, sizeof(disc));
    send_packet(buffer, sizeof(buffer), server_address_);
  }
}

bool NetworkSession::initialize_host(int port) {
  if (socket_fd_ != INVALID_SOCKET) shutdown();

#if defined(_WIN32)
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#endif

  socket_fd_ = static_cast<SOCKET>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
  if (socket_fd_ == INVALID_SOCKET) return false;

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(static_cast<uint16_t>(port));

  if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    closesocket(socket_fd_);
    socket_fd_ = INVALID_SOCKET;
    return false;
  }

   role_ = SessionRole::HOST;
   state_ = ConnectionState::CONNECTED;
   local_player_id_ = 0;
   local_car_id_ = 0;

   std::random_device rd;
   track_seed_ = rd();

  PlayerInfo host_player;
  host_player.player_id = 0;
  host_player.car_id = 0;
  host_player.type = PlayerType::HUMAN;
  host_player.state = ConnectionState::READY;
  std::snprintf(host_player.name, kMaxPlayerNameLength, "%s", "Host");
  players_[0] = host_player;

  return true;
}

bool NetworkSession::initialize_client(const std::string& host_address, int port) {
  if (socket_fd_ != INVALID_SOCKET) shutdown();

#if defined(_WIN32)
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#endif

  socket_fd_ = static_cast<SOCKET>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
  if (socket_fd_ == INVALID_SOCKET) return false;

   sockaddr_in server_addr{};
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons(static_cast<uint16_t>(port));
   inet_pton(AF_INET, host_address.c_str(), &server_addr.sin_addr);

   server_address_ = server_addr;
   has_server_address_ = true;

   JoinRequest req;
  std::memset(&req, 0, sizeof(req));
  std::snprintf(req.player_name, kMaxPlayerNameLength, "%s", "Player");
  req.requested_car_id = -1;

  if (!send_packet(&req, sizeof(req), server_addr)) {
    closesocket(socket_fd_);
    socket_fd_ = INVALID_SOCKET;
    return false;
  }

  role_ = SessionRole::CLIENT;
  state_ = ConnectionState::CONNECTING;

  return true;
}

void NetworkSession::shutdown() {
  if (socket_fd_ != INVALID_SOCKET) {
    closesocket(socket_fd_);
    socket_fd_ = INVALID_SOCKET;
  }
#if defined(_WIN32)
  WSACleanup();
#endif
  role_ = SessionRole::NONE;
  state_ = ConnectionState::DISCONNECTED;
  players_.clear();
  pending_packets_.clear();
}

void NetworkSession::send_input(const input::InputState& input, int car_id, uint32_t sequence, double timestamp) {
  if (socket_fd_ == INVALID_SOCKET) return;

  InputPacket packet;
  pack_input(input, car_id, sequence, timestamp, packet);

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  if (is_host()) {
    auto it = client_addresses_.find(car_id);
    if (it != client_addresses_.end()) {
      addr = it->second;
    } else {
      return;
    }
  } else if (has_server_address_) {
    addr = server_address_;
  } else {
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(static_cast<uint16_t>(kServerPort));
  }

   send_packet(&packet, sizeof(packet), addr);
}

void NetworkSession::broadcast_snapshot(const WorldSnapshot& snapshot) {
  if (socket_fd_ == INVALID_SOCKET || !is_host()) return;

  uint8_t buffer[kMaxPacketSize];
  buffer[0] = static_cast<uint8_t>(PacketType::SNAPSHOT);
  std::memcpy(buffer + 1, &snapshot, sizeof(WorldSnapshot));

  for (const auto& [pid, player] : players_) {
    if (pid == 0) continue;
    if (player.state < ConnectionState::CONNECTED) continue;

    auto it = client_addresses_.find(pid);
    if (it != client_addresses_.end()) {
      send_packet(buffer, static_cast<int>(sizeof(WorldSnapshot) + 1), it->second);
    }
  }
}

void NetworkSession::send_snapshot_to_client(int player_id, const WorldSnapshot& snapshot) {
  if (socket_fd_ == INVALID_SOCKET || !is_host()) return;

  uint8_t buffer[kMaxPacketSize];
  buffer[0] = static_cast<uint8_t>(PacketType::SNAPSHOT);
  std::memcpy(buffer + 1, &snapshot, sizeof(WorldSnapshot));

  auto it = client_addresses_.find(player_id);
  if (it != client_addresses_.end()) {
    send_packet(buffer, static_cast<int>(sizeof(WorldSnapshot) + 1), it->second);
  }
}

void NetworkSession::send_packet_to_all(const void* data, int len) {
  if (socket_fd_ == INVALID_SOCKET || !is_host()) return;

  for (const auto& [pid, player] : players_) {
    if (pid == 0) continue;
    if (player.state < ConnectionState::CONNECTED) continue;

    auto it = client_addresses_.find(pid);
    if (it != client_addresses_.end()) {
      send_packet(data, len, it->second);
    }
  }
}

void NetworkSession::update(double delta_time) {
  if (socket_fd_ == INVALID_SOCKET) return;

  uint8_t buffer[kMaxPacketSize];
  sockaddr_in from{};
  int from_len = sizeof(from);

  while (true) {
    int received = recvfrom(socket_fd_, reinterpret_cast<char*>(buffer), kMaxPacketSize, 0,
                            reinterpret_cast<sockaddr*>(&from), &from_len);
    if (received <= 0) break;

    PacketType type = static_cast<PacketType>(buffer[0]);

    if (is_host()) {
      if (type == PacketType::INPUT && received >= sizeof(InputPacket)) {
        pending_packets_.push_back(Packet());
        pending_packets_.back().size = received;
        std::memcpy(pending_packets_.back().data, buffer, received);
      } else if (type == PacketType::JOIN_REQUEST && received >= sizeof(JoinRequest)) {
        auto* req = reinterpret_cast<JoinRequest*>(buffer + 1);

        int assigned_car = -1;
        for (int i = 0; i < kMaxCars; ++i) {
          bool slot_taken = false;
          for (const auto& [pid, p] : players_) {
            if (p.car_id == i) { slot_taken = true; break; }
          }
          if (!slot_taken) { assigned_car = i; break; }
        }

        if (assigned_car >= 0) {
          int new_pid = static_cast<int>(players_.size());
          PlayerInfo info;
          info.player_id = new_pid;
          info.car_id = assigned_car;
          info.type = PlayerType::HUMAN;
          info.state = ConnectionState::READY;
          std::snprintf(info.name, kMaxPlayerNameLength, "%s", req->player_name);
          players_[new_pid] = info;
          client_addresses_[new_pid] = from;

          JoinAccept accept;
         accept.assigned_car_id = assigned_car;
         accept.player_id = new_pid;
         std::memset(accept.session_id, 0, sizeof(accept.session_id));
         accept.track_seed = track_seed_;

           uint8_t resp[sizeof(JoinAccept) + 1];
           resp[0] = static_cast<uint8_t>(PacketType::JOIN_ACCEPT);
           std::memcpy(resp + 1, &accept, sizeof(accept));

           send_packet(resp, sizeof(resp), from);

           if (on_player_joined_) on_player_joined_(info);
          if (on_lobby_update_) {
            std::vector<LobbySlot> slots;
            for (int i = 0; i < kMaxCars; ++i) {
              LobbySlot slot;
              slot.grid_slot = i;
              slot.occupied = false;
              for (const auto& [pid, p] : players_) {
                if (p.car_id == i) { slot.player = p; slot.occupied = true; break; }
              }
              slots.push_back(slot);
            }
            on_lobby_update_(slots);
          }
        } else {
          JoinReject reject;
          reject.reason = 1;
          uint8_t resp[2] = { static_cast<uint8_t>(PacketType::JOIN_REJECT), reject.reason };
          send_packet(resp, sizeof(resp), from);
        }
      } else if (type == PacketType::DISCONNECT && received >= sizeof(DisconnectPacket) + 1) {
        int leaving_pid = -1;
        for (auto it = client_addresses_.begin(); it != client_addresses_.end(); ++it) {
          if (it->second.sin_addr.s_addr == from.sin_addr.s_addr &&
              it->second.sin_port == from.sin_port) {
            leaving_pid = it->first;
            break;
          }
        }
        if (leaving_pid >= 0) {
          players_.erase(leaving_pid);
          client_addresses_.erase(leaving_pid);
          if (on_player_left_) on_player_left_(leaving_pid);
          if (on_lobby_update_) {
            std::vector<LobbySlot> slots;
            for (int i = 0; i < kMaxCars; ++i) {
              LobbySlot slot;
              slot.grid_slot = i;
              slot.occupied = false;
              for (const auto& [pid, p] : players_) {
                if (p.car_id == i) { slot.player = p; slot.occupied = true; break; }
              }
              slots.push_back(slot);
            }
            on_lobby_update_(slots);
          }
        }
      }
    } else {
      if (type == PacketType::JOIN_ACCEPT && received >= sizeof(JoinAccept) + 1) {
        auto* accept = reinterpret_cast<JoinAccept*>(buffer + 1);
        local_car_id_ = accept->assigned_car_id;
        local_player_id_ = accept->player_id;
        state_ = ConnectionState::READY;
      } else if (type == PacketType::JOIN_REJECT) {
        state_ = ConnectionState::DISCONNECTED;
      } else if (type == PacketType::SNAPSHOT && received >= static_cast<int>(sizeof(WorldSnapshot) + 1)) {
        auto* snap = reinterpret_cast<WorldSnapshot*>(buffer + 1);
        if (on_snapshot_) on_snapshot_(*snap);
      } else if (type == PacketType::RACE_START && received >= sizeof(RaceStartPacket) + 1) {
        auto* start = reinterpret_cast<RaceStartPacket*>(buffer + 1);
        if (on_race_start_) on_race_start_(*start);
      } else if (type == PacketType::DISCONNECT && received >= sizeof(DisconnectPacket) + 1) {
        auto* disc = reinterpret_cast<DisconnectPacket*>(buffer + 1);
        (void)disc;
        if (on_player_left_) on_player_left_(local_player_id_);
        state_ = ConnectionState::DISCONNECTED;
      }
    }
  }
}

}
