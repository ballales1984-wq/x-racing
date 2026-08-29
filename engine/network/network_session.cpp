#include "network/network_session.h"
#include "network/snapshot.h"
#include "input/input.h"

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
  if (socket_fd_ < 0) return false;
  int sent = sendto(socket_fd_, reinterpret_cast<const char*>(data), len, 0,
                    reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
  return sent == len;
}

NetworkSession::NetworkSession() = default;

NetworkSession::~NetworkSession() {
  shutdown();
}

bool NetworkSession::initialize_host(int port) {
  if (socket_fd_ >= 0) shutdown();

#if defined(_WIN32)
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#endif

  socket_fd_ = static_cast<int>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
  if (socket_fd_ < 0) return false;

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(static_cast<uint16_t>(port));

  if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    closesocket(socket_fd_);
    socket_fd_ = -1;
    return false;
  }

  role_ = SessionRole::HOST;
  state_ = ConnectionState::CONNECTED;
  local_player_id_ = 0;
  local_car_id_ = 0;

  PlayerInfo host_player;
  host_player.player_id = 0;
  host_player.car_id = 0;
  host_player.type = PlayerType::HUMAN;
  host_player.state = ConnectionState::READY;
  std::strcpy(host_player.name, "Host");
  players_[0] = host_player;

  return true;
}

bool NetworkSession::initialize_client(const std::string& host_address, int port) {
  if (socket_fd_ >= 0) shutdown();

#if defined(_WIN32)
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
#endif

  socket_fd_ = static_cast<int>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
  if (socket_fd_ < 0) return false;

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(static_cast<uint16_t>(port));
  inet_pton(AF_INET, host_address.c_str(), &server_addr.sin_addr);

  JoinRequest req;
  std::memset(&req, 0, sizeof(req));
  std::strncpy(req.player_name, "Player", kMaxPlayerNameLength - 1);
  req.requested_car_id = -1;

  if (!send_packet(&req, sizeof(req), server_addr)) {
    return false;
  }

  role_ = SessionRole::CLIENT;
  state_ = ConnectionState::CONNECTING;

  return true;
}

void NetworkSession::shutdown() {
  if (socket_fd_ >= 0) {
    closesocket(socket_fd_);
    socket_fd_ = -1;
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
  if (socket_fd_ < 0) return;

  InputPacket packet;
  pack_input(input, car_id, sequence, timestamp, packet);

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  if (is_host()) {
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(static_cast<uint16_t>(kClientPort));
  } else {
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(static_cast<uint16_t>(kServerPort));
  }

   send_packet(&packet, sizeof(packet), addr);
}

void NetworkSession::broadcast_snapshot(const WorldSnapshot& snapshot) {
  if (socket_fd_ < 0 || !is_host()) return;

  uint8_t buffer[kMaxPacketSize];
  buffer[0] = static_cast<uint8_t>(PacketType::SNAPSHOT);
  std::memcpy(buffer + 1, &snapshot, sizeof(WorldSnapshot));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(static_cast<uint16_t>(kClientPort));

  for (const auto& [pid, player] : players_) {
    if (pid == 0) continue;
    if (player.state < ConnectionState::CONNECTED) continue;
    send_packet(buffer, static_cast<int>(sizeof(WorldSnapshot) + 1), addr);
  }
}

void NetworkSession::send_snapshot_to_client(int player_id, const WorldSnapshot& snapshot) {
  if (socket_fd_ < 0 || !is_host()) return;

  uint8_t buffer[kMaxPacketSize];
  buffer[0] = static_cast<uint8_t>(PacketType::SNAPSHOT);
  std::memcpy(buffer + 1, &snapshot, sizeof(WorldSnapshot));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(static_cast<uint16_t>(kClientPort));

  send_packet(buffer, static_cast<int>(sizeof(WorldSnapshot) + 1), addr);
}

void NetworkSession::update(double delta_time) {
  if (socket_fd_ < 0) return;

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
          std::strncpy(info.name, req->player_name, kMaxPlayerNameLength - 1);
          players_[new_pid] = info;

          JoinAccept accept;
          accept.assigned_car_id = assigned_car;
          accept.player_id = new_pid;
          std::memset(accept.session_id, 0, sizeof(accept.session_id));
          accept.track_seed = 12345;

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
      }
    }
  }
}

}
