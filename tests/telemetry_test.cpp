// Project 0 — unit tests for telemetry recording and CSV export
#include <gtest/gtest.h>
#include "telemetry/telemetry.h"
#include "vehicle/vehicle.h"
#include "common.h"
#include <fstream>
#include <cstdlib>

using namespace p0;

// Telemetry should record and store frame data correctly
TEST(TelemetryV2, Recording) {
  telemetry::Telemetry tel;
  vehicle::VehicleState state;
  state.position = Vec2(10.0, 20.0);
  state.speed = 60.0;
  state.throttle = 0.5;
  state.steer_angle = 0.1;
  tel.record(state, 1.0 / 120.0);
  ASSERT_EQ(tel.frames().size(), 1u);
  EXPECT_DOUBLE_EQ(tel.frames()[0].speed, 60.0);
  EXPECT_DOUBLE_EQ(tel.frames()[0].position.x(), 10.0);
}

// Telemetry should accumulate time correctly
TEST(TelemetryV2, TimeAccumulation) {
  telemetry::Telemetry tel;
  vehicle::VehicleState state;
  state.speed = 50.0;
  tel.record(state, 1.0 / 60.0);
  tel.record(state, 1.0 / 60.0);
  tel.record(state, 1.0 / 60.0);
  ASSERT_EQ(tel.frames().size(), 3u);
  EXPECT_DOUBLE_EQ(tel.frames()[0].time, 0.0);
  EXPECT_NEAR(tel.frames()[1].time, 1.0 / 60.0, 1e-9);
  EXPECT_NEAR(tel.frames()[2].time, 2.0 / 60.0, 1e-9);
}

// Telemetry should compute lateral_g when speed > 0
TEST(TelemetryV2, LateralGComputed) {
  telemetry::Telemetry tel;
  vehicle::VehicleState state;
  state.speed = 30.0;
  state.heading = 0.0;
  state.acceleration = Vec2(0.0, 5.0);
  tel.record(state, 1.0 / 120.0);
  ASSERT_EQ(tel.frames().size(), 1u);
  EXPECT_NEAR(tel.frames()[0].lateral_g, 5.0 / kGravity, 0.01);
}

// Telemetry should clear correctly
TEST(TelemetryV2, ClearResets) {
  telemetry::Telemetry tel;
  vehicle::VehicleState state;
  state.speed = 50.0;
  tel.record(state, 1.0 / 60.0);
  tel.record(state, 1.0 / 60.0);
  EXPECT_EQ(tel.frames().size(), 2u);
  tel.clear();
  EXPECT_EQ(tel.frames().size(), 0u);
}

// Telemetry CSV should be valid and readable
TEST(TelemetryV2, CSVExport) {
  telemetry::Telemetry tel;
  vehicle::VehicleState state;
  state.position = Vec2(100.0, 200.0);
  state.speed = 80.0;
  state.rpm = 3500;
  state.gear = 3;
  state.throttle = 0.8;
  state.brake = 0.0;
  state.steer_angle = 0.05;
  state.heading = 1.0;
  state.front_tire_temp = 330.0;
  state.rear_tire_temp = 325.0;
  state.front_tire_wear = 0.95;
  state.rear_tire_wear = 0.97;
  tel.record(state, 1.0 / 60.0);

  std::string path = "D:/x-racing/data/telemetry/test_export.csv";
  tel.save_csv(path);

  std::ifstream file(path);
  ASSERT_TRUE(file.is_open());

  std::string header;
  std::getline(file, header);
  EXPECT_TRUE(header.find("time") != std::string::npos);
  EXPECT_TRUE(header.find("speed") != std::string::npos);

  std::string data_line;
  std::getline(file, data_line);
  EXPECT_TRUE(data_line.find("80") != std::string::npos);
  file.close();

  std::remove(path.c_str());
}

// Telemetry should copy all state fields
TEST(TelemetryV2, CopiesAllFields) {
  telemetry::Telemetry tel;
  vehicle::VehicleState state;
  state.speed = 55.0;
  state.rpm = 4200;
  state.gear = 2;
  state.throttle = 0.7;
  state.brake = 0.2;
  state.steer_angle = 0.03;
  state.slip_angle = 0.02;
  state.slip_ratio = 0.01;
  state.front_tire_temp = 335.0;
  state.rear_tire_temp = 330.0;
  state.front_tire_wear = 0.98;
  state.rear_tire_wear = 0.99;
  state.distance_along_track = 123.4;
  state.lap = 1;
  tel.record(state, 1.0 / 60.0);

  const auto& f = tel.frames()[0];
  EXPECT_DOUBLE_EQ(f.speed, 55.0);
  EXPECT_DOUBLE_EQ(f.rpm, 4200);
  EXPECT_EQ(f.gear, 2);
  EXPECT_DOUBLE_EQ(f.throttle, 0.7);
  EXPECT_DOUBLE_EQ(f.brake, 0.2);
  EXPECT_DOUBLE_EQ(f.steer, 0.03);
  EXPECT_DOUBLE_EQ(f.front_tire_temp, 335.0);
  EXPECT_DOUBLE_EQ(f.rear_tire_wear, 0.99);
  EXPECT_DOUBLE_EQ(f.distance, 123.4);
  EXPECT_EQ(f.gear, 2);
}

// Empty telemetry should have no frames
TEST(TelemetryV2, EmptyInitially) {
  telemetry::Telemetry tel;
  EXPECT_EQ(tel.frames().size(), 0u);
}

// Telemetry should handle zero dt gracefully
TEST(TelemetryV2, ZeroDt) {
  telemetry::Telemetry tel;
  vehicle::VehicleState state;
  state.speed = 40.0;
  tel.record(state, 0.0);
  ASSERT_EQ(tel.frames().size(), 1u);
  EXPECT_DOUBLE_EQ(tel.frames()[0].time, 0.0);
}

// Longitudinal g should be computed when moving forward
TEST(TelemetryV2, LongitudinalGComputed) {
  telemetry::Telemetry tel;
  vehicle::VehicleState state;
  state.speed = 20.0;
  state.heading = 0.0;
  state.velocity = Vec2(20.0, 0.0);
  state.acceleration = Vec2(3.0, 0.0);
  tel.record(state, 1.0 / 60.0);
  ASSERT_EQ(tel.frames().size(), 1u);
  EXPECT_NEAR(tel.frames()[0].longitudinal_g, 3.0 / kGravity, 0.01);
}
