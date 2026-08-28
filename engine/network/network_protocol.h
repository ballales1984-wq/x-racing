#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include "track/race_config.h"
#include <cstdint>
#include <vector>

namespace p0::network {

constexpr int kMaxPlayers = 10;
constexpr int kMaxCars = 10;
constexpr int kServerPort = 4242;
constexpr int kClientPort = 4243;
constexpr double kNetworkTickRate = 60.0;
constexpr double kNetworkTickDt = 1.0 / kNetworkTickRate;
constexpr int kInputBufferSize = 128;
constexpr int kSnapshotBufferSize = 128;
constexpr int kLobbyTimeoutSeconds = 30;
constexpr int kMaxPacketSize = 4096;
constexpr int kMaxPlayerNameLength = 32;

enum class PacketType : uint8_t {
  INPUT = 0x01,
  SNAPSHOT = 0x02,
  JOIN_REQUEST = 0x10,
  JOIN_ACCEPT = 0x11,
  JOIN_REJECT = 0x12,
  LOBBY_STATE = 0x13,
  RACE_START = 0x20,
  RACE_STATE = 0x21,
  PING = 0x30,
  PONG = 0x31,
  CAR_ASSIGN = 0x40,
  DISCONNECT = 0x50,
  CHAT = 0x60
};

enum class ConnectionState : uint8_t {
  DISCONNECTED = 0,
  CONNECTING,
  CONNECTED,
  READY,
  RACING,
  FINISHED
};

enum class PlayerType : uint8_t {
  HUMAN = 0,
  AI,
  EMPTY
};

struct PlayerInfo {
  int car_id = -1;
  int player_id = -1;
  char name[kMaxPlayerNameLength] = {};
  PlayerType type = PlayerType::EMPTY;
  ConnectionState state = ConnectionState::DISCONNECTED;
  double last_ping_time = 0.0;
  double ping_ms = 0.0;
  bool ready = false;
};

struct LobbySlot {
  PlayerInfo player;
  int grid_slot = -1;
  bool occupied = false;
};

struct JoinRequest {
  char player_name[kMaxPlayerNameLength];
  int requested_car_id;
};

struct JoinAccept {
  int assigned_car_id;
  int player_id;
  char session_id[16];
  uint32_t track_seed;
};

struct JoinReject {
  uint8_t reason;
};

struct CarAssignment {
  int car_id;
  int player_id;
  PlayerType type;
  int grid_slot;
  char driver_name[kMaxPlayerNameLength];
};

struct RaceStartPacket {
  double timestamp;
  double countdown_duration;
  uint32_t track_seed;
};

struct RaceStateUpdate {
  p0::race::RaceSessionState session_state;
  double race_time;
  int current_lap;
};

struct DisconnectPacket {
  uint8_t reason;
};

}
