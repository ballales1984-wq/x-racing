#include <gtest/gtest.h>
#include "network/snapshot.h"
#include "vehicle/vehicle.h"
#include "input/input.h"

using namespace p0;
using namespace p0::network;
using namespace p0::vehicle;
using namespace p0::input;
using p0::Vec2;

TEST(InputPacket, PackUnpackRoundTrip) {
  InputState in;
  in.throttle = 0.8;
  in.brake = 0.3;
  in.steering = -0.5;
  in.upshift = true;
  in.downshift = false;
  in.reset = true;
  in.enter_exit_box = false;
  in.reverse = true;

  InputPacket packet;
  pack_input(in, 7, 42, 1234.5, packet);

  EXPECT_EQ(packet.car_id, 7);
  EXPECT_EQ(packet.sequence, 42u);
  EXPECT_DOUBLE_EQ(packet.timestamp, 1234.5);
  EXPECT_FLOAT_EQ(packet.throttle, 0.8f);
  EXPECT_FLOAT_EQ(packet.brake, 0.3f);
  EXPECT_FLOAT_EQ(packet.steering, -0.5f);
  EXPECT_TRUE(packet.upshift);
  EXPECT_FALSE(packet.downshift);
  EXPECT_TRUE(packet.reset);
  EXPECT_FALSE(packet.enter_exit_box);
  EXPECT_TRUE(packet.reverse);

  InputState out;
  unpack_input(packet, out);

  EXPECT_FLOAT_EQ(out.throttle, 0.8f);
  EXPECT_FLOAT_EQ(out.brake, 0.3f);
  EXPECT_FLOAT_EQ(out.steering, -0.5f);
  EXPECT_TRUE(out.upshift);
  EXPECT_FALSE(out.downshift);
  EXPECT_TRUE(out.reset);
  EXPECT_FALSE(out.enter_exit_box);
  EXPECT_TRUE(out.reverse);
}

TEST(InputPacket, ClampsValues) {
  InputState in;
  in.throttle = 1.5;
  in.brake = -0.2;
  in.steering = 2.0;

  InputPacket packet;
  pack_input(in, 0, 0, 0.0, packet);

  EXPECT_FLOAT_EQ(packet.throttle, 1.0f);
  EXPECT_FLOAT_EQ(packet.brake, 0.0f);
  EXPECT_FLOAT_EQ(packet.steering, 1.0f);
}

TEST(Snapshot, PackUnpackRoundTrip) {
  VehicleState state;
  state.position = Vec2(100.0, 200.0);
  state.velocity = Vec2(-5.0, 10.0);
  state.heading = 1.2;
  state.speed = 25.0;
  state.rpm = 5500.0;
  state.gear = 3;
  state.steer_angle = 0.15;
  state.throttle = 0.6;
  state.brake = 0.1;
  state.lateral_g = 0.8;
  state.lap = 2;
  state.distance_along_track = 1234.5;
  state.in_box_lane = true;
  state.track_temp = 320.0;
  state.front_tire_temp = 330.0;
  state.rear_tire_temp = 325.0;
  state.front_tire_wear = 0.9;
  state.rear_tire_wear = 0.85;
  state.box_lane_entry_requested = true;

  WorldSnapshot snap;
  pack_snapshot(state, 5, snap);

  ASSERT_EQ(snap.car_count, 1);
  const CarSnapshot& cs = snap.cars[0];
  EXPECT_EQ(cs.car_id, 5);
  EXPECT_FLOAT_EQ(cs.position_x, 100.0f);
  EXPECT_FLOAT_EQ(cs.position_y, 200.0f);
  EXPECT_FLOAT_EQ(cs.velocity_x, -5.0f);
  EXPECT_FLOAT_EQ(cs.velocity_y, 10.0f);
  EXPECT_FLOAT_EQ(cs.heading, 1.2f);
  EXPECT_FLOAT_EQ(cs.speed, 25.0f);
  EXPECT_FLOAT_EQ(cs.rpm, 5500.0f);
  EXPECT_EQ(cs.gear, 3);
  EXPECT_FLOAT_EQ(cs.steer_angle, 0.15f);
  EXPECT_FLOAT_EQ(cs.throttle, 0.6f);
  EXPECT_FLOAT_EQ(cs.brake, 0.1f);
  EXPECT_FLOAT_EQ(cs.lateral_g, 0.8f);
  EXPECT_EQ(cs.lap, 2);
  EXPECT_FLOAT_EQ(cs.distance_along_track, 1234.5f);
  EXPECT_TRUE(cs.in_box_lane);
  EXPECT_FLOAT_EQ(cs.track_temp, 320.0f);
  EXPECT_FLOAT_EQ(cs.front_tire_temp, 330.0f);
  EXPECT_FLOAT_EQ(cs.rear_tire_temp, 325.0f);
  EXPECT_FLOAT_EQ(cs.front_tire_wear, 0.9f);
  EXPECT_FLOAT_EQ(cs.rear_tire_wear, 0.85f);
  EXPECT_TRUE(cs.flags & 0x01);
  EXPECT_TRUE(cs.flags & 0x02);

  VehicleState recovered;
  unpack_snapshot(cs, recovered);

  EXPECT_DOUBLE_EQ(recovered.position.x(), 100.0);
  EXPECT_DOUBLE_EQ(recovered.position.y(), 200.0);
  EXPECT_FLOAT_EQ(recovered.heading, 1.2f);
  EXPECT_DOUBLE_EQ(recovered.speed, 25.0);
  EXPECT_DOUBLE_EQ(recovered.gear, 3);
  EXPECT_TRUE(recovered.in_box_lane);
  EXPECT_TRUE(recovered.box_lane_entry_requested);
}

TEST(Snapshot, PackUpdatesExistingCar) {
  WorldSnapshot snap;
  CarSnapshot first;
  first.car_id = 1;
  first.position_x = 10.0f;
  snap.cars[0] = first;
  snap.car_count = 1;

  VehicleState state;
  state.position = Vec2(99.0, 88.0);
  state.speed = 50.0;
  pack_snapshot(state, 1, snap);

  ASSERT_EQ(snap.car_count, 1);
  EXPECT_FLOAT_EQ(snap.cars[0].position_x, 99.0f);
  EXPECT_FLOAT_EQ(snap.cars[0].speed, 50.0f);
}

TEST(Snapshot, PackAppendsNewCar) {
  WorldSnapshot snap;
  VehicleState s1;
  s1.position = Vec2(1.0, 1.0);
  s1.speed = 10.0;
  pack_snapshot(s1, 1, snap);

  VehicleState s2;
  s2.position = Vec2(2.0, 2.0);
  s2.speed = 20.0;
  pack_snapshot(s2, 2, snap);

  ASSERT_EQ(snap.car_count, 2);
  EXPECT_EQ(snap.cars[0].car_id, 1);
  EXPECT_EQ(snap.cars[1].car_id, 2);
  EXPECT_FLOAT_EQ(snap.cars[1].position_x, 2.0f);
}

TEST(Snapshot, PackRespectsMaxCars) {
  WorldSnapshot snap;
  for (int i = 0; i < kMaxCars; ++i) {
    VehicleState s;
    s.position = Vec2(i, 0.0);
    s.speed = 10.0;
    pack_snapshot(s, i, snap);
  }
  EXPECT_EQ(snap.car_count, kMaxCars);

  VehicleState extra;
  extra.position = Vec2(999.0, 999.0);
  extra.speed = 99.0;
  pack_snapshot(extra, 999, snap);
  EXPECT_EQ(snap.car_count, kMaxCars);
}

TEST(Snapshot, Sizes) {
  WorldSnapshot snap;
  snap.car_count = 2;
  EXPECT_GT(snapshot_size(snap), 0u);
  EXPECT_EQ(input_packet_size(), sizeof(InputPacket));
}
