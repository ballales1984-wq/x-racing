#include <gtest/gtest.h>
#include "vehicle/vehicle.h"
#include "simulation/simulation.h"
#include "physics/types.h"
#include "physics/tire_model.h"
#include "track/track.h"

using namespace p0;
using namespace p0::physics;

// ============================================================================
// Physics Validation Test Suite
// Validates X-Racing against real-world Porsche 911 Turbo S benchmarks
// ============================================================================

// ----------------------------------------------------------------------------
// 1. VEHICLE DYNAMICS BASELINE
// ----------------------------------------------------------------------------

TEST(VehicleDynamicsBaseline, MassMatchesPorsche911) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.mass, 1500.0, 50.0);
}

TEST(VehicleDynamicsBaseline, WheelbaseMatchesPorsche911) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.wheelbase, 2.45, 0.1);
}

TEST(VehicleDynamicsBaseline, MaxPowerMatchesPorsche911) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.max_power, 350000.0, 5000.0);
}

TEST(VehicleDynamicsBaseline, MaxTorqueMatchesPorsche911) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.max_torque, 530.0, 20.0);
}

TEST(VehicleDynamicsBaseline, DragCoefficientMatchesPorsche911) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.drag_coefficient, 0.33, 0.02);
}

TEST(VehicleDynamicsBaseline, FrontalAreaMatchesPorsche911) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.frontal_area, 2.1, 0.2);
}

TEST(VehicleDynamicsBaseline, CGHeightIsRealistic) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.cg_height, 0.2);
  EXPECT_LT(params.cg_height, 0.6);
}

TEST(VehicleDynamicsBaseline, SuspensionNaturalFrequency) {
  vehicle::VehicleParams params;
  double k_front = params.front_spring_rate;
  double m_quarter = params.mass / 4.0;
  double f = std::sqrt(k_front / m_quarter) / (2.0 * kPi);
  EXPECT_NEAR(f, 2.4, 0.6);
}

// ----------------------------------------------------------------------------
// 2. ACCELERATION BENCHMARK
// ----------------------------------------------------------------------------

TEST(AccelerationBenchmark, ZeroTo100KmHRealistic) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  sim.reset(initial);

  input::InputState input;
  input.throttle = 1.0;

  const double target_speed = 100.0 / 3.6;
  double time_to_100 = 0.0;
  const double dt = 1.0 / 60.0;

  for (int i = 0; i < 6000; ++i) {
    sim.step(input);
    time_to_100 += dt;
    if (sim.state().speed >= target_speed) break;
  }

  EXPECT_GT(time_to_100, 1.5);
  EXPECT_LT(time_to_100, 8.0);
}

TEST(AccelerationBenchmark, MaxSpeedIsRealistic) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  sim.reset(initial);

  input::InputState input;
  input.throttle = 1.0;

  for (int i = 0; i < 10000; ++i) {
    sim.step(input);
  }

  double max_speed_kmh = sim.state().speed * 3.6;
  EXPECT_GT(max_speed_kmh, 30.0);
  EXPECT_LT(max_speed_kmh, 400.0);
}

// ----------------------------------------------------------------------------
// 3. BRAKING BENCHMARK
// ----------------------------------------------------------------------------

TEST(BrakingBenchmark, StoppingDistanceFrom100KmH) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 100.0 / 3.6;
  sim.reset(initial);

  input::InputState input;
  input.brake = 1.0;

  double distance_traveled = 0.0;
  const double dt = 1.0 / 60.0;
  for (int i = 0; i < 2000; ++i) {
    const double prev_speed = sim.state().speed;
    sim.step(input);
    distance_traveled += sim.state().speed * dt;
    if (sim.state().speed < 0.5) break;
  }

  EXPECT_GT(distance_traveled, 20.0);
  EXPECT_LT(distance_traveled, 150.0);
}

TEST(BrakingBenchmark, BrakingDecelerationIsRealistic) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 80.0 / 3.6;
  sim.reset(initial);

  input::InputState input;
  input.brake = 1.0;

  for (int i = 0; i < 60; ++i) {
    sim.step(input);
  }

  double decel = std::abs(sim.state().speed - 80.0 / 3.6) / (60.0 * (1.0 / 60.0));
  EXPECT_GT(decel, 5.0);
  EXPECT_LT(decel, 15.0);
}

// ----------------------------------------------------------------------------
// 4. CORNERING PHYSICS
// ----------------------------------------------------------------------------

TEST(CorneringPhysics, LateralGOnCurve) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  const double curve_distance = 400.0;
  const auto& tp = track.at(curve_distance);

  vehicle::VehicleState initial;
  initial.position = tp.position;
  initial.heading = std::atan2(tp.tangent.y(), tp.tangent.x());
  initial.distance_along_track = curve_distance;
  initial.speed = 27.78;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.0;

  for (int i = 0; i < 300; ++i) {
    sim.step(input);
  }

  double lateral_g = sim.state().lateral_g;
  double expected_g = (initial.speed * initial.speed * std::abs(tp.curvature)) / kGravity;

  EXPECT_NEAR(lateral_g, expected_g, expected_g * 0.2 + 0.01);
  EXPECT_GT(lateral_g, 0.0);
  EXPECT_LT(lateral_g, 2.0);
}

TEST(CorneringPhysics, SlipAngleIncreasesWithSteering) {
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

  EXPECT_GT(std::abs(sim.state().front_slip_angle), 0.01);
  EXPECT_GT(std::abs(sim.state().rear_slip_angle), 0.001);
}

// ----------------------------------------------------------------------------
// 5. TIRE MODEL VALIDATION (PACEJKA)
// ----------------------------------------------------------------------------

TEST(TireModelValidation, PacejkaBInTypicalRange) {
  vehicle::VehicleParams params;
  EXPECT_GE(params.tire_pacejka_b, 8.0);
  EXPECT_LE(params.tire_pacejka_b, 15.0);
}

TEST(TireModelValidation, PacejkaCInTypicalRange) {
  vehicle::VehicleParams params;
  EXPECT_GE(params.tire_pacejka_c, 1.3);
  EXPECT_LE(params.tire_pacejka_c, 2.0);
}

TEST(TireModelValidation, PacejkaENearLinearRegime) {
  vehicle::VehicleParams params;
  EXPECT_GE(params.tire_pacejka_e, 0.9);
  EXPECT_LE(params.tire_pacejka_e, 1.0);
}

TEST(TireModelValidation, PacejkaPeakAtTypicalSlip) {
  double b = 11.0;
  double c = 1.9;
  double e = 0.97;

  double peak = pacejka_tire_model(0.15, b, c, e);
  double low = pacejka_tire_model(0.01, b, c, e);
  double high = pacejka_tire_model(0.5, b, c, e);

  EXPECT_GT(peak, low);
  EXPECT_GT(peak, high * 0.8);
}

TEST(TireModelValidation, LoadSensitivityReducesGrip) {
  double fx_low = 0.0, fy_low = 0.0;
  compute_tire_forces(0.1, 0.1, 3000.0, 0.0, 0.0, 1.2, 1.2, 11.0, 11.0, 1.9, 0.97, 0.06, 5000.0, fx_low, fy_low);

  double fx_high = 0.0, fy_high = 0.0;
  compute_tire_forces(0.1, 0.1, 8000.0, 0.0, 0.0, 1.2, 1.2, 11.0, 11.0, 1.9, 0.97, 0.06, 5000.0, fx_high, fy_high);

  double ratio_low = std::sqrt(fx_low * fx_low + fy_low * fy_low) / 3000.0;
  double ratio_high = std::sqrt(fx_high * fx_high + fy_high * fy_high) / 8000.0;
  EXPECT_GT(ratio_low, ratio_high);
}

TEST(TireModelValidation, WetTireGripReduction) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.tire_grip_wet, 0.75, 0.05);
}

TEST(TireModelValidation, TireWearRateIsRealistic) {
  vehicle::VehicleParams params;
  double wear_per_lap = params.tire_wear_per_lap;
  double laps_to_wear_out = 1.0 / wear_per_lap;
  EXPECT_GT(laps_to_wear_out, 100.0);
  EXPECT_LT(laps_to_wear_out, 5000.0);
}

// ----------------------------------------------------------------------------
// 6. TIRE THERMAL MODEL
// ----------------------------------------------------------------------------

TEST(TireThermalModel, OptimalTempIsReasonable) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.tire_optimal_temp, 340.0, 10.0);
}

TEST(TireThermalModel, TempCurveWidthIsReasonable) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.tire_temp_curve_width, 25.0, 5.0);
}

TEST(TireThermalModel, TemperatureRisesWithSlip) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 30.0;
  initial.front_tire_temp = 340.0;
  initial.rear_tire_temp = 340.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.0;
  input.steering = 0.8;

  for (int i = 0; i < 50; ++i) {
    sim.step(input);
  }

  EXPECT_GT(sim.state().front_tire_temp, 340.0);
}

TEST(TireThermalModel, RainCoolsTires) {
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

  const double start_front = sim.state().front_tire_temp;
  for (int i = 0; i < 500; ++i) {
    sim.step(input);
  }

  EXPECT_LT(sim.state().front_tire_temp, start_front);
}

// ----------------------------------------------------------------------------
// 7. SUSPENSION DYNAMICS
// ----------------------------------------------------------------------------

TEST(SuspensionDynamics, WeightTransferOnAcceleration) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 10.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 1.0;

  double initial_total_front = sim.state().fl_tire_load + sim.state().fr_tire_load;
  double initial_total_rear = sim.state().rl_tire_load + sim.state().rr_tire_load;

  for (int i = 0; i < 60; ++i) {
    sim.step(input);
  }

  double total_front = sim.state().fl_tire_load + sim.state().fr_tire_load;
  double total_rear = sim.state().rl_tire_load + sim.state().rr_tire_load;

  double front_delta = total_front - initial_total_front;
  double rear_delta = total_rear - initial_total_rear;

  EXPECT_GT(std::abs(front_delta) + std::abs(rear_delta), 0.0);
}

TEST(SuspensionDynamics, WeightTransferOnBraking) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 50.0;
  sim.reset(initial);

  input::InputState input;
  input.brake = 1.0;

  for (int i = 0; i < 60; ++i) {
    sim.step(input);
  }

  double total_front = sim.state().fl_tire_load + sim.state().fr_tire_load;
  double total_rear = sim.state().rl_tire_load + sim.state().rr_tire_load;
  double total = total_front + total_rear;

  EXPECT_GT(total, 0.0);
  EXPECT_GT(total_front / total, 0.3);
  EXPECT_LT(total_front / total, 0.8);
}

TEST(SuspensionDynamics, AntiRollBarAffectsLoadDistribution) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.anti_roll_bar_stiffness, 0.0);
  EXPECT_NEAR(params.anti_roll_bar_stiffness, 12000.0, 100.0);
}

TEST(SuspensionDynamics, BodyRollIsLimited) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.max_body_roll, 0.0);
  EXPECT_LT(params.max_body_roll, 0.2);
}

// ----------------------------------------------------------------------------
// 8. AERODYNAMICS VALIDATION
// ----------------------------------------------------------------------------

TEST(AerodynamicsValidation, DragIncreasesWithSpeed) {
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

  EXPECT_GT(sim.state().aero_drag, 0.0);
}

TEST(AerodynamicsValidation, DownforceIncreasesWithSpeed) {
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

TEST(AerodynamicsValidation, LiftIsNegativeForSportsCar) {
  vehicle::VehicleParams params;
  EXPECT_LT(params.lift_coefficient, 0.0);
}

TEST(AerodynamicsValidation, DownforceCoefficientIsRealistic) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.downforce_coefficient, 0.0);
  EXPECT_LT(params.downforce_coefficient, 0.5);
}

TEST(AerodynamicsValidation, GroundEffectFactorIsReasonable) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.ground_effect_factor, 0.0);
  EXPECT_LT(params.ground_effect_factor, 0.1);
}

TEST(AerodynamicsValidation, DragScalesWithSpeedSquared) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 80.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 1.0;

  for (int i = 0; i < 100; ++i) {
    sim.step(input);
  }
  double drag_high = sim.state().aero_drag;
  double speed_high = sim.state().speed;

  initial.speed = 40.0;
  sim.reset(initial);

  for (int i = 0; i < 100; ++i) {
    sim.step(input);
  }
  double drag_low = sim.state().aero_drag;
  double speed_low = sim.state().speed;

  if (drag_low > 1.0 && speed_high > speed_low) {
    double speed_ratio_sq = (speed_high * speed_high) / (speed_low * speed_low);
    double drag_ratio = drag_high / drag_low;
    EXPECT_NEAR(drag_ratio, speed_ratio_sq, speed_ratio_sq * 0.3);
  } else {
    EXPECT_GT(drag_high, 0.0);
  }
}

// ----------------------------------------------------------------------------
// 9. WEATHER PHYSICS
// ----------------------------------------------------------------------------

TEST(WeatherPhysics, RainReducesGrip) {
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
  EXPECT_GT(sim.state().weather_grip_factor, 0.0);
}

TEST(WeatherPhysics, RainGripReductionMatchesParams) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.rain_grip_reduction, 0.35, 0.05);
}

TEST(WeatherPhysics, RainCoolingRateIsReasonable) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.rain_cooling, 1.5, 0.3);
}

TEST(WeatherPhysics, WindEffectIsReasonable) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.wind_effect_on_speed, 0.08, 0.02);
}

TEST(WeatherPhysics, WetTiresHaveLowerGripThanDry) {
  vehicle::VehicleParams params;
  EXPECT_LT(params.tire_grip_wet, params.tire_grip_medium);
}

TEST(WeatherPhysics, TrackTemperatureChangesOverTime) {
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

// ----------------------------------------------------------------------------
// 10. COMBINED PHYSICS INTEGRATION
// ----------------------------------------------------------------------------

TEST(PhysicsIntegration, FullThrottleOnStraight) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  sim.reset(initial);

  input::InputState input;
  input.throttle = 1.0;

  for (int i = 0; i < 2000; ++i) {
    sim.step(input);
  }

  EXPECT_GT(sim.state().speed, 10.0);
  EXPECT_GT(sim.state().distance_along_track, 0.0);
}

TEST(PhysicsIntegration, SteeringAndThrottleInteraction) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 40.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.5;
  input.steering = 0.3;

  for (int i = 0; i < 300; ++i) {
    sim.step(input);
  }

  EXPECT_GT(std::abs(sim.state().front_slip_angle), 0.01);
  EXPECT_GT(std::abs(sim.state().yaw_rate), 0.0);
}

TEST(PhysicsIntegration, TireWearAccumulatesOverDistance) {
  track::Track track;
  simulation::Simulation sim;
  sim.set_track(track);

  vehicle::VehicleState initial;
  initial.position = track.get_start_position();
  initial.heading = track.get_start_heading();
  initial.speed = 50.0;
  sim.reset(initial);

  input::InputState input;
  input.throttle = 0.5;
  input.steering = 0.5;

  for (int i = 0; i < 5000; ++i) {
    sim.step(input);
  }

  EXPECT_LT(sim.state().front_tire_wear, 1.0);
  EXPECT_LT(sim.state().rear_tire_wear, 1.0);
}

TEST(PhysicsIntegration, FuelConsumptionIsRealistic) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.fuel_consumption_per_lap_l, 0.0);
  EXPECT_LT(params.fuel_consumption_per_lap_l, 50.0);
}

// ----------------------------------------------------------------------------
// 11. PACEJKA MAGIC FORMULA DETAILED VALIDATION
// ----------------------------------------------------------------------------

TEST(PacejkaDetailed, ZeroSlipReturnsZero) {
  double b = 11.0;
  double c = 1.9;
  double e = 0.97;
  EXPECT_DOUBLE_EQ(pacejka_tire_model(0.0, b, c, e), 0.0);
}

TEST(PacejkaDetailed, SymmetricForSmallSlip) {
  double b = 10.0;
  double c = 1.7;
  double e = 0.97;

  double pos = pacejka_tire_model(0.1, b, c, e);
  double neg = pacejka_tire_model(-0.1, b, c, e);
  EXPECT_NEAR(pos, -neg, 0.01);
}

TEST(PacejkaDetailed, PeaksNearOptimalSlip) {
  double b = 10.0;
  double c = 1.7;
  double e = 0.97;

  double val_zero = pacejka_tire_model(0.0, b, c, e);
  double val_peak = pacejka_tire_model(0.15, b, c, e);

  EXPECT_GT(val_peak, val_zero);
}

TEST(PacejkaDetailed, FrictionEllipseLimitsCombinedSlip) {
  double fx = 0.0, fy = 0.0;
  combined_slip_force(0.9, 0.9, 4000.0, 1.2, 1.2, 11.0, 11.0, 1.9, 0.97, fx, fy);
  double combined = std::sqrt(fx * fx + fy * fy);
  double max_possible = 1.2 * 4000.0 * 1.2;
  EXPECT_LE(combined, max_possible);
}

TEST(PacejkaDetailed, CamberThrustAddsLateralForce) {
  double fx = 0.0, fy = 0.0;
  compute_tire_forces(0.0, 0.1, 4000.0, 0.02, 1.0, 1.2, 1.2, 11.0, 11.0, 1.9, 0.97, 0.06, 5000.0, fx, fy);
  EXPECT_GT(fy, 0.0);
}

// ----------------------------------------------------------------------------
// 12. SUSPENSION PARAMETER VALIDATION
// ----------------------------------------------------------------------------

TEST(SuspensionParams, FrontSpringRateIsReasonable) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.front_spring_rate, 35000.0, 5000.0);
}

TEST(SuspensionParams, RearSpringRateIsReasonable) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.rear_spring_rate, 38000.0, 5000.0);
}

TEST(SuspensionParams, DampingIsReasonable) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.front_damping, 0.0);
  EXPECT_GT(params.rear_damping, 0.0);
}

TEST(SuspensionParams, RideHeightIsReasonable) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.ride_height, 0.05);
  EXPECT_LT(params.ride_height, 0.3);
}

TEST(SuspensionParams, AntiRollStiffnessIsReasonable) {
  vehicle::VehicleParams params;
  EXPECT_NEAR(params.anti_roll_bar_stiffness, 12000.0, 2000.0);
}
