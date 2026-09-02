#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include "network/network_protocol.h"
#include "input/input.h"
#include <cstdint>
#include <cstring>

namespace p0::network {

using p0::race::RaceSessionState;

struct CarSnapshot {
  int car_id = -1;
  float position_x = 0.0f;
  float position_y = 0.0f;
  float velocity_x = 0.0f;
  float velocity_y = 0.0f;
  float heading = 0.0f;
  float speed = 0.0f;
  float rpm = 0.0f;
  int32_t gear = 1;
  float steer_angle = 0.0f;
  float throttle = 0.0f;
  float brake = 0.0f;
  float lateral_g = 0.0f;
  int32_t lap = 0;
  float distance_along_track = 0.0f;
  bool in_box_lane = false;
  float track_temp = 305.0f;
  float front_tire_temp = 300.0f;
  float rear_tire_temp = 300.0f;
  float front_tire_wear = 1.0f;
  float rear_tire_wear = 1.0f;
  uint32_t flags = 0;
};

struct WorldSnapshot {
  double timestamp = 0.0;
  uint32_t sequence = 0;
  p0::race::RaceSessionState session_state = p0::race::RaceSessionState::PREGAME;
  int current_lap = 0;
  double race_time = 0.0;
  int car_count = 0;
  CarSnapshot cars[kMaxCars];
};

struct InputPacket {
  int car_id = 0;
  uint32_t sequence = 0;
  double timestamp = 0.0;
  float throttle = 0.0f;
  float brake = 0.0f;
  float steering = 0.0f;
  bool upshift = false;
  bool downshift = false;
  bool reset = false;
  bool enter_exit_box = false;
  bool reverse = false;
  uint8_t padding[2] = {};
};

inline void pack_input(const input::InputState& input, int car_id, uint32_t seq, double ts, InputPacket& out) {
  out.car_id = car_id;
  out.sequence = seq;
  out.timestamp = ts;
  out.throttle = static_cast<float>(clamp(input.throttle, 0.0, 1.0));
  out.brake = static_cast<float>(clamp(input.brake, 0.0, 1.0));
  out.steering = static_cast<float>(clamp(input.steering, -1.0, 1.0));
  out.upshift = input.upshift;
  out.downshift = input.downshift;
  out.reset = input.reset;
   out.enter_exit_box = input.enter_exit_box;
   out.reverse = input.reverse;
}

inline void unpack_input(const InputPacket& packet, input::InputState& out) {
  out.throttle = static_cast<double>(packet.throttle);
  out.brake = static_cast<double>(packet.brake);
  out.steering = static_cast<double>(packet.steering);
  out.upshift = packet.upshift;
  out.downshift = packet.downshift;
  out.reset = packet.reset;
   out.enter_exit_box = packet.enter_exit_box;
   out.reverse = packet.reverse;
}

inline void pack_snapshot(const vehicle::VehicleState& state, int car_id, WorldSnapshot& out) {
  CarSnapshot snap;
  snap.car_id = car_id;
  snap.position_x = static_cast<float>(state.position.x());
  snap.position_y = static_cast<float>(state.position.y());
  snap.velocity_x = static_cast<float>(state.velocity.x());
  snap.velocity_y = static_cast<float>(state.velocity.y());
  snap.heading = static_cast<float>(state.heading);
  snap.speed = static_cast<float>(state.speed);
  snap.rpm = static_cast<float>(state.rpm);
  snap.gear = static_cast<int32_t>(state.gear);
  snap.steer_angle = static_cast<float>(state.steer_angle);
  snap.throttle = static_cast<float>(state.throttle);
  snap.brake = static_cast<float>(state.brake);
  snap.lateral_g = static_cast<float>(state.lateral_g);
  snap.lap = static_cast<int32_t>(state.lap);
  snap.distance_along_track = static_cast<float>(state.distance_along_track);
  snap.in_box_lane = state.in_box_lane;
  snap.track_temp = static_cast<float>(state.track_temp);
  snap.front_tire_temp = static_cast<float>(state.front_tire_temp);
  snap.rear_tire_temp = static_cast<float>(state.rear_tire_temp);
  snap.front_tire_wear = static_cast<float>(state.front_tire_wear);
  snap.rear_tire_wear = static_cast<float>(state.rear_tire_wear);
  if (state.in_box_lane) snap.flags |= 0x01;
  if (state.box_lane_entry_requested) snap.flags |= 0x02;

  bool found = false;
  for (int i = 0; i < out.car_count; ++i) {
    if (out.cars[i].car_id == car_id) {
      out.cars[i] = snap;
      found = true;
      break;
    }
  }
  if (!found && out.car_count < kMaxCars) {
    out.cars[out.car_count] = snap;
    out.car_count++;
  }
}

inline void unpack_snapshot(const CarSnapshot& snap, vehicle::VehicleState& out) {
  out.position = Vec2(static_cast<double>(snap.position_x), static_cast<double>(snap.position_y));
  out.velocity = Vec2(static_cast<double>(snap.velocity_x), static_cast<double>(snap.velocity_y));
  out.heading = static_cast<double>(snap.heading);
  out.speed = static_cast<double>(snap.speed);
  out.rpm = static_cast<double>(snap.rpm);
  out.gear = static_cast<int>(snap.gear);
  out.steer_angle = static_cast<double>(snap.steer_angle);
  out.throttle = static_cast<double>(snap.throttle);
  out.brake = static_cast<double>(snap.brake);
  out.lateral_g = static_cast<double>(snap.lateral_g);
  out.lap = static_cast<int>(snap.lap);
  out.distance_along_track = static_cast<double>(snap.distance_along_track);
  out.in_box_lane = (snap.flags & 0x01) != 0;
  out.box_lane_entry_requested = (snap.flags & 0x02) != 0;
  out.track_temp = static_cast<double>(snap.track_temp);
  out.front_tire_temp = static_cast<double>(snap.front_tire_temp);
  out.rear_tire_temp = static_cast<double>(snap.rear_tire_temp);
  out.front_tire_wear = static_cast<double>(snap.front_tire_wear);
  out.rear_tire_wear = static_cast<double>(snap.rear_tire_wear);
}

inline size_t snapshot_size(const WorldSnapshot& snap) {
  return sizeof(WorldSnapshot) - (kMaxCars - snap.car_count) * sizeof(CarSnapshot);
}

inline size_t input_packet_size() {
  return sizeof(InputPacket);
}

}
