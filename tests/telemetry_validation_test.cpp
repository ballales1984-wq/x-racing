// Project 0 — comprehensive telemetry validation tests
// Covers: recording, time accumulation, g-force computation, CSV export,
//         lap tracking, tracked telemetry (surface-aware), and edge cases.
#include <gtest/gtest.h>
#include <cmath>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>

#include "telemetry/telemetry.h"
#include "tracking/tracked_telemetry.h"
#include "tracking/track_position.h"
#include "tracking/position_sample.h"
#include "tracking/lap_system.h"
#include "vehicle/vehicle.h"
#include "common.h"

using namespace p0;

// ---------------------------------------------------------------------------
// Section 1 — Basic vehicle telemetry
// ---------------------------------------------------------------------------

TEST(TelemetryValidation, FrameDefaultsAreZero) {
  telemetry::TelemetryFrame frame{};
  EXPECT_DOUBLE_EQ(frame.time, 0.0);
  EXPECT_EQ(frame.lap_number, 0);
  EXPECT_DOUBLE_EQ(frame.distance, 0.0);
  EXPECT_DOUBLE_EQ(frame.speed, 0.0);
  EXPECT_DOUBLE_EQ(frame.rpm, 0.0);
  EXPECT_EQ(frame.gear, 0);
  EXPECT_DOUBLE_EQ(frame.throttle, 0.0);
  EXPECT_DOUBLE_EQ(frame.brake, 0.0);
  EXPECT_DOUBLE_EQ(frame.steer, 0.0);
  EXPECT_DOUBLE_EQ(frame.slip_angle, 0.0);
  EXPECT_DOUBLE_EQ(frame.slip_ratio, 0.0);
  EXPECT_DOUBLE_EQ(frame.position.x(), 0.0);
  EXPECT_DOUBLE_EQ(frame.position.y(), 0.0);
  EXPECT_DOUBLE_EQ(frame.heading, 0.0);
  EXPECT_DOUBLE_EQ(frame.lateral_g, 0.0);
  EXPECT_DOUBLE_EQ(frame.longitudinal_g, 0.0);
}

TEST(TelemetryValidation, RecordsAllScalarFields) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 55.5;
  s.rpm = 4500.0;
  s.gear = 4;
  s.throttle = 0.85;
  s.brake = 0.1;
  s.steer_angle = 0.07;
  s.slip_angle = 0.03;
  s.slip_ratio = 0.05;
  s.distance_along_track = 250.0;
  s.lap = 2;
  s.heading = 0.4;
  s.front_tire_temp = 340.0;
  s.rear_tire_temp = 335.0;
  s.front_tire_wear = 0.92;
  s.rear_tire_wear = 0.95;
  s.position = Vec2(15.0, -25.0);
  s.velocity = Vec2(40.0, 0.0);
  s.acceleration = Vec2(0.0, 0.0);

  tel.record(s, 1.0 / 120.0);
  ASSERT_EQ(tel.frames().size(), 1u);

  const auto& f = tel.frames()[0];
  EXPECT_DOUBLE_EQ(f.speed, 55.5);
  EXPECT_DOUBLE_EQ(f.rpm, 4500.0);
  EXPECT_EQ(f.gear, 4);
  EXPECT_DOUBLE_EQ(f.throttle, 0.85);
  EXPECT_DOUBLE_EQ(f.brake, 0.1);
  EXPECT_DOUBLE_EQ(f.steer, 0.07);
  EXPECT_DOUBLE_EQ(f.slip_angle, 0.03);
  EXPECT_DOUBLE_EQ(f.slip_ratio, 0.05);
  EXPECT_DOUBLE_EQ(f.distance, 250.0);
  EXPECT_EQ(f.lap_number, 2);
  EXPECT_DOUBLE_EQ(f.heading, 0.4);
  EXPECT_DOUBLE_EQ(f.front_tire_temp, 340.0);
  EXPECT_DOUBLE_EQ(f.rear_tire_temp, 335.0);
  EXPECT_DOUBLE_EQ(f.front_tire_wear, 0.92);
  EXPECT_DOUBLE_EQ(f.rear_tire_wear, 0.95);
  EXPECT_DOUBLE_EQ(f.position.x(), 15.0);
  EXPECT_DOUBLE_EQ(f.position.y(), -25.0);
  EXPECT_DOUBLE_EQ(f.velocity.x(), 40.0);
}

TEST(TelemetryValidation, FirstFrameStartsAtZeroTime) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 10.0;
  tel.record(s, 1.0 / 60.0);
  EXPECT_DOUBLE_EQ(tel.frames()[0].time, 0.0);
}

TEST(TelemetryValidation, TimeAccumulatesAcrossFrames) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 20.0;
  const double dt = 1.0 / 120.0;
  for (int i = 0; i < 5; ++i) tel.record(s, dt);
  ASSERT_EQ(tel.frames().size(), 5u);
  for (int i = 0; i < 5; ++i) {
    EXPECT_NEAR(tel.frames()[i].time, i * dt, 1e-12);
  }
}

TEST(TelemetryValidation, TimeAccumulationIrregularDt) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 20.0;
  tel.record(s, 0.5);
  tel.record(s, 0.25);
  tel.record(s, 1.5);
  // Frame 0 starts at 0. Subsequent frames accumulate dt cumulatively.
  EXPECT_NEAR(tel.frames()[0].time, 0.0,   1e-9);
  EXPECT_NEAR(tel.frames()[1].time, 0.25,  1e-9);
  EXPECT_NEAR(tel.frames()[2].time, 1.75,  1e-9);
}

TEST(TelemetryValidation, ZeroDtDoesNotAdvanceTime) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  tel.record(s, 0.0);
  tel.record(s, 0.0);
  ASSERT_EQ(tel.frames().size(), 2u);
  EXPECT_DOUBLE_EQ(tel.frames()[0].time, 0.0);
  EXPECT_DOUBLE_EQ(tel.frames()[1].time, 0.0);
}

TEST(TelemetryValidation, NegativeDtIsNotClamped) {
  // Documenting behaviour: a negative dt causes time to go backwards.
  // This test asserts the math is consistent (no clamping, no exception).
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  tel.record(s, 1.0);
  tel.record(s, -0.25);
  EXPECT_NEAR(tel.frames()[1].time, -0.25, 1e-12);
}

// ---------------------------------------------------------------------------
// Section 2 — g-force computation
// ---------------------------------------------------------------------------

TEST(TelemetryValidation, LateralGZeroWhenStationary) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 0.0;          // stationary
  s.acceleration = Vec2(0.0, 10.0);
  tel.record(s, 1.0 / 60.0);
  // speed <= epsilon -> g-forces not computed
  EXPECT_DOUBLE_EQ(tel.frames()[0].lateral_g, 0.0);
  EXPECT_DOUBLE_EQ(tel.frames()[0].longitudinal_g, 0.0);
}

TEST(TelemetryValidation, LateralGOnlyWhenTurning) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 25.0;
  s.heading = 0.0;                       // facing +x
  s.velocity = Vec2(25.0, 0.0);
  s.acceleration = Vec2(0.0, 9.80665);   // pure +y (lateral axis)
  tel.record(s, 1.0 / 60.0);
  EXPECT_NEAR(tel.frames()[0].lateral_g, 1.0, 1e-3);
  EXPECT_NEAR(tel.frames()[0].longitudinal_g, 0.0, 1e-6);
}

TEST(TelemetryValidation, LongitudinalGPositiveAccelerating) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 30.0;
  s.heading = 0.0;
  s.velocity = Vec2(30.0, 0.0);
  s.acceleration = Vec2(9.80665, 0.0);  // pure +x (forward)
  tel.record(s, 1.0 / 60.0);
  EXPECT_NEAR(tel.frames()[0].longitudinal_g, 1.0, 1e-3);
}

TEST(TelemetryValidation, LongitudinalGNegativeBraking) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 30.0;
  s.heading = 0.0;
  s.velocity = Vec2(30.0, 0.0);
  s.acceleration = Vec2(-9.80665, 0.0);
  tel.record(s, 1.0 / 60.0);
  EXPECT_NEAR(tel.frames()[0].longitudinal_g, -1.0, 1e-3);
}

TEST(TelemetryValidation, CombinedLatLongGDecomposed) {
  // Acceleration vector with equal forward (+x) and lateral (+y) components.
  // Both projections onto the forward and lateral axes equal 'a', so both
  // g-forces should equal 1.0.
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 20.0;
  s.heading = 0.0;
  s.velocity = Vec2(20.0, 0.0);
  const double a = 9.80665;
  s.acceleration = Vec2(a, a);
  tel.record(s, 1.0 / 60.0);
  EXPECT_NEAR(tel.frames()[0].longitudinal_g, 1.0, 1e-3);
  EXPECT_NEAR(tel.frames()[0].lateral_g,       1.0, 1e-3);
}

// ---------------------------------------------------------------------------
// Section 3 — Lap tracking
// ---------------------------------------------------------------------------

TEST(TelemetryValidation, MarkLapOverridesLapNumber) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 10.0;
  s.lap = 0;
  tel.record(s, 1.0 / 60.0);
  EXPECT_EQ(tel.frames()[0].lap_number, 0);

  tel.mark_lap(1);
  s.lap = 1;
  tel.record(s, 1.0 / 60.0);
  // mark_lap sets current_lap_; the next frame pulls from state.lap
  // since record() copies state.lap -> frame.lap_number after mark_lap().
  EXPECT_EQ(tel.frames()[1].lap_number, 1);

  tel.mark_lap(2);
  s.lap = 2;
  tel.record(s, 1.0 / 60.0);
  EXPECT_EQ(tel.frames()[2].lap_number, 2);
}

TEST(TelemetryValidation, LapIncrementsDuringLongRun) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 20.0;
  for (int i = 0; i < 4; ++i) {
    s.lap = i;
    tel.record(s, 1.0 / 60.0);
  }
  ASSERT_EQ(tel.frames().size(), 4u);
  EXPECT_EQ(tel.frames()[0].lap_number, 0);
  EXPECT_EQ(tel.frames()[1].lap_number, 1);
  EXPECT_EQ(tel.frames()[2].lap_number, 2);
  EXPECT_EQ(tel.frames()[3].lap_number, 3);
}

// ---------------------------------------------------------------------------
// Section 4 — Clear / reset semantics
// ---------------------------------------------------------------------------

TEST(TelemetryValidation, ClearRemovesAllFrames) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 50.0;
  for (int i = 0; i < 10; ++i) tel.record(s, 1.0 / 60.0);
  EXPECT_EQ(tel.frames().size(), 10u);
  tel.clear();
  EXPECT_TRUE(tel.frames().empty());
}

TEST(TelemetryValidation, ClearResetsLapCounter) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 30.0;
  tel.mark_lap(3);
  tel.record(s, 1.0 / 60.0);
  tel.clear();
  s.lap = 0;
  tel.record(s, 1.0 / 60.0);
  EXPECT_EQ(tel.frames()[0].lap_number, 0);
}

TEST(TelemetryValidation, RecordingResumesAfterClear) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 10.0;
  tel.record(s, 0.5);
  tel.clear();
  // After clear the next frame starts a fresh time base at 0.
  tel.record(s, 0.25);
  EXPECT_DOUBLE_EQ(tel.frames()[0].time, 0.0);
}

// ---------------------------------------------------------------------------
// Section 5 — CSV export
// ---------------------------------------------------------------------------

static std::string read_file(const std::string& path) {
  std::ifstream f(path);
  std::stringstream ss;
  ss << f.rdbuf();
  return ss.str();
}

static void safe_remove(const std::string& path) {
  // Use std::remove (C) which does not throw — avoids filesystem::remove
  // throwing while a previous ifstream is still holding the file open.
  std::remove(path.c_str());
}

TEST(TelemetryValidation, CSVExportHeaderIncludesAllColumns) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 20.0;
  tel.record(s, 1.0 / 60.0);
  const std::string path = "data/telemetry/test_validation_header.csv";
  tel.save_csv(path);

  const std::string content = read_file(path);
  const std::vector<std::string> expected = {
    "time", "lap_number", "distance", "speed", "rpm", "gear",
    "throttle", "brake", "steer", "slip_angle", "slip_ratio",
    "pos_x", "pos_y", "vel_x", "vel_y", "acc_x", "acc_y",
    "heading", "lateral_g", "longitudinal_g",
    "front_tire_temp", "rear_tire_temp", "front_tire_wear", "rear_tire_wear"
  };
  std::string line;
  std::getline(std::ifstream(path), line);
  for (const auto& col : expected) {
    EXPECT_NE(line.find(col), std::string::npos) << "missing column: " << col;
  }
  safe_remove(path);
}

TEST(TelemetryValidation, CSVExportRowCountMatchesFrames) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 20.0;
  for (int i = 0; i < 7; ++i) tel.record(s, 1.0 / 60.0);

  const std::string path = "data/telemetry/test_validation_rows.csv";
  tel.save_csv(path);

  std::ifstream f(path);
  std::string line;
  int data_lines = 0;
  std::getline(f, line);  // skip header
  while (std::getline(f, line)) {
    if (!line.empty()) ++data_lines;
  }
  EXPECT_EQ(data_lines, 7);
  safe_remove(path);
}

TEST(TelemetryValidation, CSVExportEmptyProducesHeaderOnly) {
  telemetry::Telemetry tel;
  const std::string path = "data/telemetry/test_validation_empty.csv";
  tel.save_csv(path);

  std::ifstream f(path);
  ASSERT_TRUE(f.is_open());
  std::string line;
  std::getline(f, line);
  EXPECT_NE(line.find("time"), std::string::npos);
  EXPECT_FALSE(std::getline(f, line));  // no data
  safe_remove(path);
}

TEST(TelemetryValidation, CSVExportInvalidPathIsSafe) {
  // Should not throw or crash when the path cannot be opened.
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 10.0;
  tel.record(s, 1.0 / 60.0);
  EXPECT_NO_THROW(tel.save_csv("Z:/nonexistent_dir/should_fail.csv"));
}

TEST(TelemetryValidation, CSVExportFormatsNumbersWithSixDecimals) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 12.3456789;
  s.throttle = 0.1111111;
  tel.record(s, 1.0 / 60.0);

  const std::string path = "data/telemetry/test_validation_format.csv";
  tel.save_csv(path);

  const std::string content = read_file(path);
  // 12.345679 (rounded to 6 decimals) should appear; 0.111111 likewise.
  EXPECT_NE(content.find("12.345679"), std::string::npos);
  EXPECT_NE(content.find("0.111111"), std::string::npos);
  safe_remove(path);
}

// ---------------------------------------------------------------------------
// Section 6 — Tracked telemetry (surface-aware / GPS enriched)
// ---------------------------------------------------------------------------

TEST(TelemetryValidation, TrackedTelemetryFrameDefaults) {
  tracking::TrackedTelemetry t;
  EXPECT_TRUE(t.frames().empty());
}

TEST(TelemetryValidation, TrackedTelemetryCapturesAllSources) {
  tracking::TrackedTelemetry t("test_tracked_all.csv");

  tracking::TrackPosition pos{};
  pos.s = 250.0;
  pos.lateral = 0.7;
  pos.heading = 0.4;
  pos.on_track = true;

  tracking::PositionSample gps{};
  gps.timestamp = 12.5;
  gps.latitude = 45.123;
  gps.longitude = 11.456;
  gps.altitude = 100.0;
  gps.speed = 50.0;
  gps.heading = 0.4;
  gps.horizontal_accuracy = 1.2;
  gps.valid = true;

  tracking::LapSystem lap(1000.0, 3);
  pos.s = 0.0;
  lap.update(pos, 0.0);
  pos.s = 250.0;
  lap.update(pos, 10.0);

  t.record(pos, gps, 0.85, 78.5, 0.2, lap);
  ASSERT_EQ(t.frames().size(), 1u);

  const auto& f = t.frames()[0];
  EXPECT_DOUBLE_EQ(f.timestamp, 12.5);
  EXPECT_DOUBLE_EQ(f.s, 250.0);
  EXPECT_DOUBLE_EQ(f.lateral, 0.7);
  EXPECT_DOUBLE_EQ(f.speed, 50.0);
  EXPECT_DOUBLE_EQ(f.heading, 0.4);
  EXPECT_DOUBLE_EQ(f.latitude, 45.123);
  EXPECT_DOUBLE_EQ(f.longitude, 11.456);
  EXPECT_DOUBLE_EQ(f.altitude, 100.0);
  EXPECT_DOUBLE_EQ(f.gps_speed, 50.0);
  EXPECT_DOUBLE_EQ(f.gps_heading, 0.4);
  EXPECT_DOUBLE_EQ(f.gps_accuracy, 1.2);
  EXPECT_DOUBLE_EQ(f.grip, 0.85);
  EXPECT_DOUBLE_EQ(f.surface_temp, 78.5);
  EXPECT_DOUBLE_EQ(f.rubber, 0.2);
}

TEST(TelemetryValidation, TrackedTelemetryPropagatesLapNumber) {
  tracking::TrackedTelemetry t("test_tracked_lap.csv");

  // Use a small track so the wrap is easy to drive.
  const double track_len = 100.0;
  tracking::TrackPosition pos{};
  pos.s = 0.0;
  pos.on_track = true;

  tracking::PositionSample gps{};
  gps.timestamp = 0.0;
  gps.speed = 10.0;

  tracking::LapSystem lap(track_len, 5);

  // First update initializes the detector (prev_distance = 0).
  lap.update(pos, 0.0);

  // Move to mid-track and record a frame. completed_laps is still 0.
  pos.s = 50.0;
  lap.update(pos, 1.0);
  t.record(pos, gps, 1.0, 20.0, 0.0, lap);
  EXPECT_EQ(t.frames()[0].lap_number, 0);
  EXPECT_EQ(lap.completed_laps(), 0);

  // Drive past the start/finish line in the forward direction: jump from
  // a high s to a low s. The detector should see a negative delta whose
  // magnitude exceeds track_length/2 and count it as a forward crossing.
  pos.s = 90.0;
  lap.update(pos, 2.0);
  pos.s = 5.0;
  lap.update(pos, 3.0);
  EXPECT_EQ(lap.completed_laps(), 1);

  t.record(pos, gps, 1.0, 20.0, 0.0, lap);
  EXPECT_GE(t.frames()[1].lap_number, 1);
}

TEST(TelemetryValidation, TrackedTelemetryCsvHeaderColumns) {
  tracking::TrackedTelemetry t("test_tracked_header.csv");
  tracking::TrackPosition pos{};
  pos.on_track = true;
  pos.s = 10.0;
  tracking::PositionSample gps{};
  gps.timestamp = 1.0;
  gps.speed = 30.0;
  tracking::LapSystem lap(1000.0, 1);
  lap.update(pos, 0.0);
  t.record(pos, gps, 0.9, 70.0, 0.0, lap);
  t.save_csv();

  std::ifstream f("test_tracked_header.csv");
  ASSERT_TRUE(f.is_open());
  std::string header;
  std::getline(f, header);

  const std::vector<std::string> expected = {
    "timestamp", "lap_number", "s", "lateral", "speed", "heading",
    "latitude", "longitude", "altitude", "gps_speed", "gps_heading",
    "gps_accuracy", "grip", "surface_temp", "rubber", "lap_time"
  };
  for (const auto& col : expected) {
    EXPECT_NE(header.find(col), std::string::npos) << "missing: " << col;
  }
  safe_remove("test_tracked_header.csv");
}

TEST(TelemetryValidation, TrackedTelemetryClearResetsFrames) {
  tracking::TrackedTelemetry t("test_tracked_clear.csv");
  tracking::TrackPosition pos{};
  pos.on_track = true;
  pos.s = 10.0;
  tracking::PositionSample gps{};
  gps.timestamp = 1.0;
  tracking::LapSystem lap(1000.0, 1);
  lap.update(pos, 0.0);
  t.record(pos, gps, 1.0, 20.0, 0.0, lap);
  t.record(pos, gps, 1.0, 21.0, 0.0, lap);
  EXPECT_EQ(t.frames().size(), 2u);
  t.clear();
  EXPECT_TRUE(t.frames().empty());
}

TEST(TelemetryValidation, TrackedTelemetryCsvInvalidPathIsSafe) {
  tracking::TrackedTelemetry t("Z:/nope/tracked_should_fail.csv");
  EXPECT_NO_THROW(t.save_csv());
}

// ---------------------------------------------------------------------------
// Section 7 — Long-run smoke tests (small but representative)
// ---------------------------------------------------------------------------

TEST(TelemetryValidation, LongRunPreservesTimeMonotonic) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 30.0;
  const double dt = 1.0 / 120.0;
  const int N = 600;  // 5 seconds of data
  for (int i = 0; i < N; ++i) tel.record(s, dt);
  ASSERT_EQ(tel.frames().size(), static_cast<size_t>(N));
  for (int i = 1; i < N; ++i) {
    EXPECT_GT(tel.frames()[i].time, tel.frames()[i - 1].time);
    EXPECT_NEAR(tel.frames()[i].time - tel.frames()[i - 1].time, dt, 1e-12);
  }
}

TEST(TelemetryValidation, LongRunCsvRoundTrip) {
  telemetry::Telemetry tel;
  vehicle::VehicleState s;
  s.speed = 40.0;
  s.throttle = 0.5;
  const double dt = 1.0 / 60.0;
  const int N = 100;
  for (int i = 0; i < N; ++i) tel.record(s, dt);

  const std::string path = "data/telemetry/test_validation_long.csv";
  tel.save_csv(path);

  std::ifstream f(path);
  ASSERT_TRUE(f.is_open());
  std::string line;
  int rows = 0;
  std::getline(f, line);  // header
  while (std::getline(f, line)) {
    if (!line.empty()) ++rows;
  }
  EXPECT_EQ(rows, N);
  safe_remove(path);
}
