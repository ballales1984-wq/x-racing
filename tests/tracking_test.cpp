#include "tracking/position_sample.h"
#include "tracking/track_position.h"
#include "tracking/clock.h"
#include "tracking/simulated_gps.h"
#include "tracking/real_gps.h"
#include "tracking/track_mapper.h"
#include "tracking/tracking_system.h"
#include "track/track.h"

#include <gtest/gtest.h>
#include <cmath>
#include <memory>

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
  EXPECT_TRUE(gps.update(s));
  EXPECT_TRUE(s.valid);
  EXPECT_GT(s.timestamp, 0.0);

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
  ASSERT_TRUE(gps.update(s));
  EXPECT_EQ(s.latitude, 1.0);
  EXPECT_EQ(s.longitude, 2.0);

  gps.stop();
}

TEST(Tracking, TrackMapperProducesOnTrack) {
  p0::track::Track track(p0::track::TrackType::Default);
  TrackMapper mapper(track);

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
  auto mapper = std::make_unique<TrackMapper>(track);

  TrackingSystem system(std::make_unique<SimulatedGPS>(SimulatedGPS::Params{10.0, 0.0}));
  system.set_mapper(std::move(mapper));
  ASSERT_TRUE(system.start());

  system.update();
  EXPECT_TRUE(system.is_running());
  EXPECT_TRUE(system.current_sample().valid);

  system.stop();
  EXPECT_FALSE(system.is_running());
}

}  // namespace p0::tracking
