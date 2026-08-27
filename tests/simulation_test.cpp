#include <gtest/gtest.h>
#include "vehicle/vehicle.h"
#include "track/track.h"
#include "simulation/simulation.h"
#include "telemetry/telemetry.h"
#include "physics/types.h"

// Project 0 — unit tests
// Tests verify the mathematical behavior of the simulation core.
using namespace p0;

// VehicleState should initialize with sensible defaults
TEST(VehicleState, Initialization) {
  vehicle::VehicleState state;
  EXPECT_DOUBLE_EQ(state.position.x(), 0.0);
  EXPECT_DOUBLE_EQ(state.position.y(), 0.0);
  EXPECT_DOUBLE_EQ(state.speed, 0.0);
  EXPECT_EQ(state.gear, 1);
  EXPECT_EQ(state.lap, 0);
}

// Track should have positive length and width
TEST(Track, BasicProperties) {
  track::Track track;
  EXPECT_GT(track.length(), 0.0);
  EXPECT_GT(track.at(0.0).width, 0.0);
}

// Full throttle should produce forward movement with bounded speed
TEST(Simulation, ForwardMovement) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  sim.reset(initial);

  input::InputState input;
  input.throttle = 1.0;

  for (int i = 0; i < 1000; ++i) {
    sim.step(input);
    EXPECT_GE(sim.state().speed, 0.0);
    EXPECT_LE(sim.state().speed, 150.0);
  }

  EXPECT_GT(sim.state().distance_along_track, 0.0);
}

// Telemetry should record and store frame data correctly
TEST(Telemetry, Recording) {
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

// M2: Zero steering should maintain a straight line
TEST(Steering, ZeroSteerMaintainsStraightLine) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 20.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.3;

  const double start_x = sim.state().position.x();
  const double start_y = sim.state().position.y();
  const double start_heading = sim.state().heading;

  for (int i = 0; i < 600; ++i) {
    sim.step(input);
  }

  EXPECT_NEAR(sim.state().heading, start_heading, 0.05);
  EXPECT_GT(sim.state().position.x(), start_x);
  EXPECT_NEAR(sim.state().position.y(), start_y, 3.5);
}

// M2: Constant steering at low speed should produce a coherent curve
TEST(Steering, ConstantSteerAtLowSpeed) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 10.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.3;
  input.steering = 0.5;

  const double start_heading = sim.state().heading;

  for (int i = 0; i < 600; ++i) {
    sim.step(input);
  }

  const double actual_heading_change = std::abs(normalize_angle(sim.state().heading - start_heading));
  EXPECT_GT(actual_heading_change, 0.1);
  EXPECT_LT(actual_heading_change, 2.0);
}

// M3: Steering should produce non-zero slip angles
TEST(Grip, SteeringProducesSlipAngles) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 30.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.5;

  for (int i = 0; i < 200; ++i) {
    sim.step(input);
  }

  EXPECT_NEAR(sim.state().steer_angle, 0.5 * vehicle::VehicleParams{}.max_steer_angle, 0.01);
  EXPECT_GT(std::abs(sim.state().front_slip_angle), 0.01);
}

// M3: High-speed steering should show understeer (front slip > rear slip)
TEST(Grip, HighSpeedUndersteer) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 80.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.8;

  for (int i = 0; i < 60; ++i) {
    sim.step(input);
  }

  EXPECT_GT(std::abs(sim.state().front_slip_angle), 0.05);
  EXPECT_GT(std::abs(sim.state().rear_slip_angle), 0.001);
  EXPECT_GT(std::abs(sim.state().front_slip_angle), std::abs(sim.state().rear_slip_angle));
}

// M3: Returning steering to zero should reduce slip angles
TEST(Grip, ReturningToCenterReducesSlip) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 40.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.6;

  for (int i = 0; i < 200; ++i) {
    sim.step(input);
  }

  const double slip_before = std::abs(sim.state().front_slip_angle);

  input.steering = 0.0;
  for (int i = 0; i < 300; ++i) {
    sim.step(input);
  }

  const double slip_after = std::abs(sim.state().front_slip_angle);
  EXPECT_LT(slip_after, slip_before);
}

// M4: Tire temperature should rise when slip is present
TEST(Tire, TemperatureRisesWithSlip) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 30.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.8;

  for (int i = 0; i < 300; ++i) {
    sim.step(input);
  }

  EXPECT_GT(sim.state().front_tire_temp, sim.state().rear_tire_temp);
  EXPECT_GT(sim.state().front_tire_temp, vehicle::VehicleParams{}.ambient_temperature);
}

// M4: Tire wear should decrease over time with slip
TEST(Tire, WearIncreasesWithSlip) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 50.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.9;

  for (int i = 0; i < 500; ++i) {
    sim.step(input);
  }

  EXPECT_LT(sim.state().front_tire_wear, 1.0);
  EXPECT_LT(sim.state().rear_tire_wear, 1.0);
}

// M4: Reset should restore tire temperature and wear to defaults
TEST(Tire, ResetRestoresTireState) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 50.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.9;

  for (int i = 0; i < 200; ++i) {
    sim.step(input);
  }

  sim.reset(initial);

  EXPECT_DOUBLE_EQ(sim.state().front_tire_temp, sim.state().rear_tire_temp);
  EXPECT_DOUBLE_EQ(sim.state().front_tire_wear, 1.0);
  EXPECT_DOUBLE_EQ(sim.state().rear_tire_wear, 1.0);
}

// M6: Aerodynamic drag should increase with speed
TEST(Aerodynamics, DragIncreasesWithSpeed) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 50.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 1.0;

  for (int i = 0; i < 200; ++i) {
    sim.step(input);
  }

  const double speed = sim.state().speed;
  EXPECT_GT(speed, 30.0);
  EXPECT_GT(sim.state().aero_drag, 0.0);
}

// M6: Downforce should increase with speed
TEST(Aerodynamics, DownforceIncreasesWithSpeed) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 60.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.5;

  for (int i = 0; i < 300; ++i) {
    sim.step(input);
  }

  EXPECT_GT(sim.state().aero_downforce, 0.0);
}

// M6: Higher wing angle should produce more downforce
TEST(Aerodynamics, WingAngleAffectsDownforce) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 60.0;
  sim.reset(initial);

  vehicle::VehicleParams params;
  params.wing_angle = 0.3;
  sim = simulation::Simulation(simulation::SimulationParams{});
  sim.set_track(track);
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.5;

  for (int i = 0; i < 300; ++i) {
    sim.step(input);
  }

  EXPECT_GT(sim.state().aero_downforce, 0.0);
}

// M7: Rain intensity should reduce weather grip factor
TEST(Weather, RainReducesGrip) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 50.0;
  sim.reset(initial);

  sim.mutable_params().rain_intensity = 0.5;

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.5;

  for (int i = 0; i < 100; ++i) {
    sim.step(input);
  }

  EXPECT_LT(sim.state().weather_grip_factor, 1.0);
}

// M7: Rain should cool tires
TEST(Weather, RainCoolsTires) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 50.0;
  initial.front_tire_temp = 340.0;
  initial.rear_tire_temp = 340.0;
  sim.reset(initial);

  sim.mutable_params().rain_intensity = 0.8;

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.5;

  const double start_front_temp = sim.state().front_tire_temp;
  const double start_rear_temp = sim.state().rear_tire_temp;

  for (int i = 0; i < 200; ++i) {
    sim.step(input);
  }

  EXPECT_LT(sim.state().front_tire_temp, start_front_temp);
  EXPECT_LT(sim.state().rear_tire_temp, start_rear_temp);
}

// M7: Track temperature should change over time
TEST(Weather, TrackTemperatureChanges) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 50.0;
  initial.track_temp = 300.0;
  sim.reset(initial);

  sim.mutable_params().ambient_temperature = 310.0;

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.5;

  for (int i = 0; i < 300; ++i) {
    sim.step(input);
  }

  EXPECT_GT(sim.state().track_temp, 300.0);
}

// M8: Centripetal force should be zero on a straight
TEST(Centripetal, ZeroOnStraight) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 50.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.0;

  for (int i = 0; i < 100; ++i) {
    sim.step(input);
  }

  EXPECT_DOUBLE_EQ(sim.state().centripetal_force, 0.0);
  EXPECT_DOUBLE_EQ(sim.state().centrifugal_force, 0.0);
  EXPECT_DOUBLE_EQ(sim.state().lateral_g, 0.0);
}

// M8: Centripetal force should be non-zero on a curve
TEST(Centripetal, NonZeroOnCurve) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  const double curve_distance = 800.0;
  const auto& tp = track.at(curve_distance);

  vehicle::VehicleState initial;
  initial.position = tp.position;
  initial.heading = std::atan2(tp.tangent.y(), tp.tangent.x());
  initial.distance_along_track = curve_distance;
  initial.speed = 50.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.0;

  for (int i = 0; i < 300; ++i) {
    sim.step(input);
  }

  std::cout << "DEBUG NonZero: dist=" << sim.state().distance_along_track
            << " speed=" << sim.state().speed
            << " centripetal=" << sim.state().centripetal_force
            << " lat_g=" << sim.state().lateral_g << "\n";

  EXPECT_GT(sim.state().centripetal_force, 0.0);
  EXPECT_GT(sim.state().lateral_g, 0.0);
}

// Debug: print curvature at a known curve distance
TEST(Centripetal, DebugCurvature) {
  track::Track track;
  const double curve_distance = 800.0;
  const auto& tp = track.at(curve_distance);
  std::cout << "DEBUG: distance=" << curve_distance 
            << " curvature=" << tp.curvature 
            << " normal=(" << tp.normal.x() << ", " << tp.normal.y() << ")\n";
  SUCCEED();
}

// M8: Lateral G should scale with the square of speed
TEST(Centripetal, LateralGScalesWithSpeed) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  const double curve_distance = 800.0;
  const auto& tp = track.at(curve_distance);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.0;

  // Place car on curve at low speed
  vehicle::VehicleState initial;
  initial.position = tp.position;
  initial.heading = std::atan2(tp.tangent.y(), tp.tangent.x());
  initial.distance_along_track = curve_distance;
  initial.speed = 20.0;
  sim.reset(initial);

  for (int i = 0; i < 5; ++i) {
    sim.step(input);
  }
  const double low_speed_lateral_g = sim.state().lateral_g;

  // Place car on curve at high speed
  initial.speed = 80.0;
  sim.reset(initial);

  for (int i = 0; i < 5; ++i) {
    sim.step(input);
  }
  const double high_speed_lateral_g = sim.state().lateral_g;

  const double ratio = high_speed_lateral_g / (low_speed_lateral_g + 1e-9);
  EXPECT_NEAR(ratio, 16.0, 4.0);
}

// M8: Centrifugal force should be the reaction (opposite sign to centripetal)
TEST(Centripetal, CentrifugalIsReaction) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  const double curve_distance = 800.0;
  const auto& tp = track.at(curve_distance);

  vehicle::VehicleState initial;
  initial.position = tp.position;
  initial.heading = std::atan2(tp.tangent.y(), tp.tangent.x());
  initial.distance_along_track = curve_distance;
  initial.speed = 50.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.0;

  for (int i = 0; i < 300; ++i) {
    sim.step(input);
  }

  EXPECT_DOUBLE_EQ(sim.state().centrifugal_force, -sim.state().centripetal_force);
}

// Box lane exists on the main straight
TEST(BoxLane, ExistsOnStraight) {
  track::Track track;
  EXPECT_TRUE(track.has_box_lane_at(100.0));
  EXPECT_TRUE(track.has_box_lane_at(200.0));
}

// Box lane does not exist on corners or past straight
TEST(BoxLane, AbsentOnCorners) {
  track::Track track;
  EXPECT_FALSE(track.has_box_lane_at(0.0));
  EXPECT_FALSE(track.has_box_lane_at(350.0));
}

// Vehicle can enter box lane on straight
TEST(BoxLane, VehicleEntersBoxLane) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 10.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.0;
  input.enter_exit_box = true;

  const auto& tp = track.at(0.0);
  const double box_left = -tp.width / 2.0 - tp.box_lane_width;
  const double box_right = -tp.width / 2.0;
  initial.position = tp.position + tp.normal * ((box_left + box_right) / 2.0);
  sim.reset(initial);

  for (int i = 0; i < 60; ++i) {
    sim.step(input);
  }

  EXPECT_TRUE(sim.state().in_box_lane);
  EXPECT_DOUBLE_EQ(sim.state().box_lane_speed, 22.2);
}

// Speed is limited in box lane
TEST(BoxLane, SpeedLimitedInBoxLane) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 50.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 1.0;
  input.enter_exit_box = true;

  const auto& tp = track.at(0.0);
  const double box_left = -tp.width / 2.0 - tp.box_lane_width;
  const double box_right = -tp.width / 2.0;
  initial.position = tp.position + tp.normal * ((box_left + box_right) / 2.0);
  sim.reset(initial);

  for (int i = 0; i < 120; ++i) {
    sim.step(input);
  }

  EXPECT_TRUE(sim.state().in_box_lane);
  EXPECT_LE(sim.state().speed, 22.2);
}

// M5: Surface friction coefficients are distinct per surface type
TEST(TrackCoefficient, FrictionValuesAreDistinct) {
  using namespace p0::track;
  EXPECT_GT(friction_for_surface(SurfaceType::Asphalt), friction_for_surface(SurfaceType::OldAsphalt));
  EXPECT_GT(friction_for_surface(SurfaceType::OldAsphalt), friction_for_surface(SurfaceType::Kerb));
  EXPECT_GT(friction_for_surface(SurfaceType::Kerb), friction_for_surface(SurfaceType::Dirt));
  EXPECT_GT(friction_for_surface(SurfaceType::Dirt), friction_for_surface(SurfaceType::Grass));
  EXPECT_GT(friction_for_surface(SurfaceType::Gravel), friction_for_surface(SurfaceType::Grass));
  EXPECT_GT(friction_for_surface(SurfaceType::Gravel), friction_for_surface(SurfaceType::Sand));
  EXPECT_DOUBLE_EQ(friction_for_surface(SurfaceType::Asphalt), 1.0);
  EXPECT_DOUBLE_EQ(friction_for_surface(SurfaceType::WetAsphalt), 0.7);
  EXPECT_DOUBLE_EQ(friction_for_surface(SurfaceType::Sand), 0.25);
}

// M5: Default track has per-segment surface variation
TEST(TrackCoefficient, DefaultTrackHasSurfaceVariation) {
  track::Track track;
  EXPECT_EQ(track.surface_type_at(100.0), track::SurfaceType::Asphalt);
  EXPECT_EQ(track.surface_type_at(200.0), track::SurfaceType::Asphalt);
  EXPECT_EQ(track.surface_type_at(350.0), track::SurfaceType::OldAsphalt);
  EXPECT_EQ(track.surface_type_at(700.0), track::SurfaceType::Asphalt);
}

// M5: set_surface_at changes surface type and friction at a distance
TEST(TrackCoefficient, SetSurfaceAtModifiesTrack) {
  track::Track track;
  const double len = track.length();
  const double mid = 350.0;

  EXPECT_EQ(track.surface_type_at(mid), track::SurfaceType::OldAsphalt);
  track.set_surface_at(mid, track::SurfaceType::WetAsphalt);
  EXPECT_EQ(track.surface_type_at(mid), track::SurfaceType::WetAsphalt);
  EXPECT_NEAR(track.at(mid).friction, 0.7, 1e-9);
}

// M5: Higher track friction produces more acceleration on straight
TEST(TrackCoefficient, HigherFrictionProducesMoreAcceleration) {
  track::TrackParams params;
  params.default_friction = 1.0;
  track::Track track_high(params);

  params.default_friction = 0.5;
  track::Track track_low(params);

  simulation::Simulation sim_high;
  sim_high.set_track(track_high);

  simulation::Simulation sim_low;
  sim_low.set_track(track_low);

  vehicle::VehicleState initial;
  initial.position = track_high.get_start_position();
  initial.heading = track_high.get_start_heading();
  initial.speed = 10.0;

  sim_high.reset(initial);
  vehicle::VehicleState initial_low = initial;
  initial_low.position = track_low.get_start_position();
  initial_low.heading = track_low.get_start_heading();
  sim_low.reset(initial_low);

  input::InputState input;
  input.throttle = 1.0;

  for (int i = 0; i < 300; ++i) {
    sim_high.step(input);
    sim_low.step(input);
  }

  EXPECT_GT(sim_high.state().speed, sim_low.state().speed);
}

// Tests for OffTrack physics, barrier pushback, and respawn
TEST(OffTrackAndRespawn, OffTrackDetectedAndDampened) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  const auto& tp = track.at(100.0);
  // Place car far off-track (lateral offset > half width)
  initial.position = tp.position + tp.normal * (tp.width / 2.0 + 3.0);
  initial.heading = std::atan2(tp.tangent.y(), tp.tangent.x());
  initial.distance_along_track = 100.0;
  initial.speed = 50.0;

  sim.reset(initial);
  input::InputState input;

  const auto res = sim.step(input);
  EXPECT_TRUE(res.off_track);
  // Off-track terrain should reduce speed quickly
  EXPECT_LT(sim.state().speed, 50.0);
}

TEST(OffTrackAndRespawn, RespawnRestoresValidState) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 30.0;
  sim.reset(initial);

  // Step on track to save a valid state
  input::InputState input;
  input.throttle = 0.5;
  for (int i = 0; i < 60; ++i) sim.step(input);

  const double dist_before = sim.state().distance_along_track;
  EXPECT_GT(dist_before, 0.0);

  // Now perform respawn
  sim.respawn();

  EXPECT_EQ(sim.state().speed, 0.0);
  EXPECT_NEAR(sim.state().distance_along_track, dist_before, 1e-3);
}

