#include "tracking/position_sample.h"
#include "tracking/track_position.h"
#include "tracking/clock.h"
#include "tracking/simulated_gps.h"
#include "tracking/real_gps.h"
#include "tracking/replay_gps.h"
#include "tracking/track_mapper.h"
#include "tracking/tracking_system.h"
#include "tracking/lap_system.h"
#include "tracking/tracked_telemetry.h"
#include "tracking/coordinate_converter.h"
#include "tracking/physics_trajectory.h"
#include "tracking/replay_trajectory.h"
#include "tracking/checkpoint.h"
#include "tracking/network_position_provider.h"
#include "tracking/surface_cell.h"
#include "tracking/surface_grid.h"
#include "tracking/surface_system.h"
#include "track/track.h"
#include "vehicle/vehicle.h"

#include <gtest/gtest.h>
#include <cmath>
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>

namespace p0::tracking {

class DummyTrajectory : public ITrajectorySource {
 public:
  PositionSample sample_at(double time) override {
    PositionSample s{};
    s.timestamp = time;
    s.latitude = 45.0 + time * 0.001;
    s.longitude = 11.0 + time * 0.001;
    s.altitude = 10.0;
    s.speed = 30.0;
    s.heading = 0.5;
    s.horizontal_accuracy = 1.0;
    s.valid = true;
    return s;
  }

  double duration() const override { return 100.0; }
};

TEST(Tracking, PositionSampleDefaults) {
  PositionSample sample{};
  EXPECT_FALSE(sample.valid);
  EXPECT_EQ(sample.speed, 0.0);
}

TEST(Tracking, TrackPositionDefaults) {
  TrackPosition pos{};
  EXPECT_FALSE(pos.on_track);
  EXPECT_EQ(pos.segment_index, -1);
}

TEST(Tracking, SimulationClockAdvances) {
  SimulationClock clock(0.0);
  EXPECT_EQ(clock.now(), 0.0);
  clock.advance(1.5);
  EXPECT_EQ(clock.now(), 1.5);
  clock.set_time(10.0);
  EXPECT_EQ(clock.now(), 10.0);
}

TEST(Tracking, RealClockAdvances) {
  RealClock clock;
  const double t1 = clock.now();
  EXPECT_GT(t1, 0.0);
}

TEST(Tracking, ReplayClockAdvances) {
  ReplayClock clock(0.0, 2.0);
  clock.update();
  const double t1 = clock.now();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  clock.update();
  const double t2 = clock.now();
  EXPECT_GT(t2, t1);
}

TEST(Tracking, SimulatedGPSCycles) {
  auto traj = std::make_unique<DummyTrajectory>();
  SimulatedGPS gps(std::move(traj), SimulatedGPS::Params{10.0, 0.0});
  ASSERT_TRUE(gps.start());
  EXPECT_TRUE(gps.is_running());

  PositionSample s{};
  EXPECT_TRUE(gps.poll(s));
  EXPECT_TRUE(s.valid);

  gps.stop();
  EXPECT_FALSE(gps.is_running());
}

TEST(Tracking, RealGPSWrapper) {
  class FakeDevice : public IGPSDevice {
   public:
    bool connect() override { return true; }
    void disconnect() override {}
    bool read(PositionSample& sample) override {
      sample.latitude = 1.0;
      sample.longitude = 2.0;
      sample.valid = true;
      return true;
    }
  };

  auto dev = std::make_unique<FakeDevice>();
  RealGPS gps(std::move(dev));
  ASSERT_TRUE(gps.start());

  PositionSample s{};
  ASSERT_TRUE(gps.poll(s));
  EXPECT_EQ(s.latitude, 1.0);
  EXPECT_EQ(s.longitude, 2.0);

  gps.stop();
}

TEST(Tracking, TrackMapperProducesOnTrack) {
  p0::track::Track track(p0::track::TrackType::Default);
  TrackMapper mapper(track, TrackMapper::LocalOrigin{45.0, 11.0});

  PositionSample sample{};
  sample.latitude = 45.0;
  sample.longitude = 11.0;
  sample.valid = true;

  TrackPosition pos = mapper.map(sample);
  EXPECT_TRUE(pos.on_track);
  EXPECT_GE(pos.s, 0.0);
}

TEST(Tracking, TrackingSystemUpdates) {
  auto traj = std::make_unique<DummyTrajectory>();
  SimulatedGPS gps(std::move(traj));
  ASSERT_TRUE(gps.start());

  p0::track::Track track(p0::track::TrackType::Default);
  TrackMapper::LocalOrigin origin{45.0, 11.0};
  auto mapper = std::make_unique<TrackMapper>(track, origin);

  auto traj2 = std::make_unique<DummyTrajectory>();
  TrackingSystem system(std::make_unique<SimulatedGPS>(std::move(traj2)));
  system.set_mapper(std::move(mapper));
  ASSERT_TRUE(system.start());

  system.update();
  EXPECT_TRUE(system.is_running());
  EXPECT_TRUE(system.current_sample().valid);

  system.stop();
  EXPECT_FALSE(system.is_running());
}

TEST(Tracking, LapSystemStartsAndFinishes) {
  LapSystem lap_system(1000.0, 3);

  TrackPosition pos{};
  pos.on_track = true;

  lap_system.update(pos, 0.0);
  EXPECT_TRUE(lap_system.lap_started());

  for (int i = 0; i < 10; ++i) {
    pos.s = i * 100.0;
    lap_system.update(pos, i * 0.1);
  }

  pos.s = 950.0;
  lap_system.update(pos, 1.0);
  EXPECT_FALSE(lap_system.lap_finished());

  pos.s = 1050.1;
  lap_system.update(pos, 1.1);
  EXPECT_TRUE(lap_system.lap_finished());
  EXPECT_EQ(lap_system.completed_laps(), 1);
}

TEST(Tracking, LapSystemReset) {
  LapSystem lap_system(1000.0, 3);

  TrackPosition pos{};
  pos.on_track = true;
  pos.s = 0.0;
  lap_system.update(pos, 0.0);

  pos.s = 999.9;
  lap_system.update(pos, 0.5);

  pos.s = 1000.1;
  lap_system.update(pos, 1.0);
  EXPECT_EQ(lap_system.completed_laps(), 1);

  lap_system.reset();
  EXPECT_EQ(lap_system.completed_laps(), 0);
  EXPECT_FALSE(lap_system.lap_started());
}

TEST(Tracking, TrackedTelemetryRecords) {
  TrackedTelemetry telemetry("test_tracked_telemetry.csv");

  TrackPosition pos{};
  pos.s = 100.0;
  pos.lateral = -1.0;
  pos.on_track = true;
  pos.heading = 0.5;

  PositionSample gps{};
  gps.timestamp = 12.5;
  gps.latitude = 45.1;
  gps.longitude = 11.2;
  gps.altitude = 30.0;
  gps.speed = 50.0;
  gps.heading = 0.5;
  gps.horizontal_accuracy = 1.5;
  gps.valid = true;

  LapSystem lap_system(1000.0, 3);
  lap_system.update(pos, 0.0);

  telemetry.record(pos, gps, 0.9, 85.0, 0.1, lap_system);
  ASSERT_EQ(telemetry.frames().size(), 1u);
  EXPECT_EQ(telemetry.frames()[0].s, 100.0);
  EXPECT_EQ(telemetry.frames()[0].speed, 50.0);
  EXPECT_EQ(telemetry.frames()[0].latitude, 45.1);
  EXPECT_EQ(telemetry.frames()[0].gps_accuracy, 1.5);
}

TEST(Tracking, SurfaceCellDefaults) {
  SurfaceCell cell{};
  EXPECT_EQ(cell.temperature, 20.0);
  EXPECT_EQ(cell.rubber, 0.0);
  EXPECT_EQ(cell.moisture, 0.0);
  EXPECT_EQ(cell.grip, 1.0);
}

TEST(Tracking, SurfaceGridIndexing) {
  SurfaceGrid grid(1000.0, 14.0, 1.0, 0.5);
  EXPECT_EQ(grid.s_count(), 1000);
  EXPECT_EQ(grid.l_count(), 28);

  SurfaceCell& c1 = grid.cell(0.0, 0.0);
  SurfaceCell& c2 = grid.cell(999.0, 0.0);
  c1.rubber = 1.0;
  EXPECT_EQ(grid.cell(0.0, 0.0).rubber, 1.0);
  EXPECT_EQ(c2.rubber, 0.0);
}

TEST(Tracking, SurfaceSystemDeposit) {
  p0::track::Track track(p0::track::TrackParams{});
  ASSERT_GT(track.length(), 0.0);

  SurfaceSystem system(track);
  ASSERT_GT(system.grid().s_count(), 0);
  ASSERT_GT(system.grid().l_count(), 0);

  VehicleSurfaceInput input{};
  input.track_position.s = 0.0;
  input.track_position.lateral = 0.0;
  input.on_track = true;
  input.speed = 60.0;
  input.load = 1.0;

  system.apply_vehicle(input);
  EXPECT_GT(system.grid().cell(0.0, 0.0).rubber, 0.0);
}

TEST(Tracking, SurfaceSystemGripModel) {
  p0::track::Track track(p0::track::TrackType::Default);
  SurfaceSystem system(track);

  system.set_ambient_moisture(0.8);
  system.update(1.0);

  const double grip = system.grip_at(TrackPosition{});
  EXPECT_LT(grip, 1.0);
}

TEST(Tracking, CoordinateConverterRoundTrip) {
  CoordinateConverter converter(
      GeographicOrigin{45.0, 11.0, 0.0});

  Vec2 local(123.4, -56.7);
  PositionSample sample = converter.local_to_geographic(local, 0.3, 40.0, 1.0);

  EXPECT_NEAR(sample.latitude, 45.0, 1e-2);
  EXPECT_NEAR(sample.longitude, 11.0, 1e-2);
  EXPECT_EQ(sample.altitude, 0.0);
  EXPECT_EQ(sample.heading, 0.3);
  EXPECT_EQ(sample.speed, 40.0);
  EXPECT_TRUE(sample.valid);

  Vec2 back = converter.geographic_to_local(sample.latitude, sample.longitude);
  EXPECT_NEAR(back.x(), local.x(), 1e-2);
  EXPECT_NEAR(back.y(), local.y(), 1e-2);
}

TEST(Tracking, SimulatedGPSAppliesNoise) {
  auto traj = std::make_unique<DummyTrajectory>();
  SimulatedGPS gps(std::move(traj), SimulatedGPS::Params{10.0, 5.0});
  ASSERT_TRUE(gps.start());

  PositionSample s{};
  ASSERT_TRUE(gps.poll(s));
  EXPECT_GT(s.horizontal_accuracy, 0.0);
}

TEST(Tracking, SimulatedGPSStartRequiresTrajectory) {
  SimulatedGPS gps(nullptr, SimulatedGPS::Params{10.0, 0.0});
  EXPECT_FALSE(gps.start());
}

TEST(Tracking, ReplayGPSCycles) {
  auto traj = std::make_unique<DummyTrajectory>();
  ReplayGPS gps(std::move(traj), 10.0);
  ASSERT_TRUE(gps.start());

  PositionSample s{};
  EXPECT_TRUE(gps.poll(s));
  EXPECT_TRUE(s.valid);
  EXPECT_GE(s.timestamp, 0.0);

  gps.stop();
  EXPECT_FALSE(gps.is_running());
}

TEST(Tracking, TrackMapperCenterline) {
  p0::track::Track track(p0::track::TrackType::Default);
  TrackMapper mapper(track, TrackMapper::LocalOrigin{45.0, 11.0});

  PositionSample sample{};
  sample.latitude = 45.0;
  sample.longitude = 11.0;
  sample.valid = true;

  TrackPosition pos = mapper.map(sample);
  EXPECT_TRUE(pos.on_track);
  EXPECT_NEAR(pos.lateral, 0.0, 2.0);
}

TEST(Tracking, TrackMapperRightSide) {
  p0::track::Track track(p0::track::TrackType::Default);
  TrackMapper mapper(track, TrackMapper::LocalOrigin{45.0, 11.0});

  PositionSample sample{};
  sample.latitude = 45.0;
  sample.longitude = 11.0;
  sample.valid = true;

  const double meters_per_deg_lat = 111132.92;

  sample.latitude = 45.0 - 6.0 / meters_per_deg_lat;

  TrackPosition pos = mapper.map(sample);
  EXPECT_TRUE(pos.on_track);
  EXPECT_LT(pos.lateral, 0.0);
}

TEST(Tracking, TrackMapperLeftSide) {
  p0::track::Track track(p0::track::TrackType::Default);
  TrackMapper mapper(track, TrackMapper::LocalOrigin{45.0, 11.0});

  PositionSample sample{};
  sample.latitude = 45.0;
  sample.longitude = 11.0;
  sample.valid = true;

  const double lat_rad = 45.0 * kDegToRad;
  const double meters_per_deg_lat = 111132.92;

  sample.latitude = 45.0 + 6.0 / meters_per_deg_lat;

  TrackPosition pos = mapper.map(sample);
  EXPECT_TRUE(pos.on_track);
  EXPECT_GT(pos.lateral, 0.0);
}

TEST(Tracking, TrackMapperOffTrack) {
  p0::track::Track track(p0::track::TrackType::Default);
  TrackMapper mapper(track, TrackMapper::LocalOrigin{45.0, 11.0});

  PositionSample sample{};
  sample.latitude = 45.0;
  sample.longitude = 11.0;
  sample.valid = true;

  const double lat_rad = 45.0 * kDegToRad;
  const double meters_per_deg_lat = 111132.92;
  const double meters_per_deg_lon = 111412.84 * std::cos(lat_rad);

  sample.longitude = 11.0 + 100.0 / meters_per_deg_lon;
  sample.latitude = 45.0 + 100.0 / meters_per_deg_lat;

  TrackPosition pos = mapper.map(sample);
  EXPECT_FALSE(pos.on_track);
}

TEST(Tracking, NetworkPositionProviderCycles) {
  NetworkPositionProvider provider(NetworkPositionProvider::Params{10.0});
  ASSERT_TRUE(provider.start());
  EXPECT_TRUE(provider.is_running());

  PositionSample sample{};
  sample.latitude = 45.0;
  sample.longitude = 11.0;
  sample.valid = true;
  provider.set_sample(sample);

  PositionSample out{};
  EXPECT_TRUE(provider.poll(out));
  EXPECT_EQ(out.latitude, 45.0);
  EXPECT_EQ(out.longitude, 11.0);

  provider.stop();
  EXPECT_FALSE(provider.is_running());
}

TEST(Tracking, PhysicsTrajectoryProducesSamples) {
  p0::vehicle::VehicleState state{};
  state.position = p0::Vec2(100.0, -50.0);
  state.heading = 0.5;
  state.velocity = p0::Vec2(10.0, 5.0);

  CoordinateConverter converter(GeographicOrigin{45.0, 11.0, 0.0});
  PhysicsTrajectory traj(converter, &state);

  PositionSample s = traj.sample_at(1.0);
  EXPECT_TRUE(s.valid);
  EXPECT_GT(s.latitude, 0.0);
  EXPECT_GT(s.longitude, 0.0);
  EXPECT_EQ(s.timestamp, 1.0);
}

TEST(Tracking, ReplayTrajectorySamples) {
  std::vector<ReplayTrajectory::Sample> samples;
  samples.push_back({0.0, 45.0, 11.0, 10.0, 30.0, 0.5, true});
  samples.push_back({1.0, 45.001, 11.001, 10.0, 31.0, 0.5, true});
  samples.push_back({2.0, 45.002, 11.002, 10.0, 32.0, 0.5, true});

  ReplayTrajectory traj(std::move(samples));
  ASSERT_EQ(traj.duration(), 2.0);

  PositionSample s = traj.sample_at(1.0);
  EXPECT_TRUE(s.valid);
  EXPECT_EQ(s.timestamp, 1.0);
  EXPECT_NEAR(s.latitude, 45.001, 1e-6);
}

TEST(Tracking, CheckpointSystemValidatesPassage) {
  CheckpointSystem cp_system(1000.0);
  cp_system.set_checkpoints({
    {100.0, 5.0, 0},
    {500.0, 5.0, 1},
    {900.0, 5.0, 2}
  });

  TrackPosition pos{};
  pos.on_track = true;

  pos.s = 100.0;
  auto r1 = cp_system.validate(pos);
  EXPECT_TRUE(r1.passed);
  EXPECT_EQ(cp_system.next_checkpoint(), 1);

  pos.s = 500.0;
  auto r2 = cp_system.validate(pos);
  EXPECT_TRUE(r2.passed);
  EXPECT_EQ(cp_system.next_checkpoint(), 2);

  pos.s = 900.0;
  auto r3 = cp_system.validate(pos);
  EXPECT_TRUE(r3.passed);
  EXPECT_TRUE(cp_system.all_passed());
}

TEST(Tracking, CheckpointSystemResets) {
  CheckpointSystem cp_system(1000.0);
  cp_system.set_checkpoints({{100.0, 5.0, 0}});

  TrackPosition pos{};
  pos.on_track = true;
  pos.s = 100.0;
  cp_system.validate(pos);
  EXPECT_EQ(cp_system.next_checkpoint(), 1);

  cp_system.reset();
  EXPECT_EQ(cp_system.next_checkpoint(), 0);
}

}  // namespace p0::tracking
