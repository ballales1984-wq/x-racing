// Project 0 — unit tests for physics primitives and tire model
#include <gtest/gtest.h>
#include "physics/types.h"
#include "vehicle/vehicle.h"

using namespace p0;

// Pacejka tire model should return 0 at zero slip
TEST(Pacejka, ZeroSlipReturnsZero) {
  double mu = 1.2;
  double b = 11.0;
  double c = 1.9;
  double e = 0.97;
  EXPECT_DOUBLE_EQ(physics::pacejka_tire_model(0.0, mu, b, c, e), 0.0);
}

// Pacejka should peak near optimal slip
TEST(Pacejka, PeaksNearOptimalSlip) {
  double mu = 1.0;
  double b = 10.0;
  double c = 1.7;
  double e = 0.97;

  double val_zero = physics::pacejka_tire_model(0.0, mu, b, c, e);
  double val_peak = physics::pacejka_tire_model(0.15, mu, b, c, e);

  EXPECT_GT(val_peak, val_zero);
}

// Pacejka should be symmetric-ish for small slip angles
TEST(Pacejka, SymmetricForSmallSlip) {
  double mu = 1.0;
  double b = 10.0;
  double c = 1.7;
  double e = 0.97;

  double pos = physics::pacejka_tire_model(0.1, mu, b, c, e);
  double neg = physics::pacejka_tire_model(-0.1, mu, b, c, e);
  EXPECT_NEAR(pos, -neg, 0.01);
}

// Vector projection should preserve component along vector
TEST(VectorOps, ProjectOntoVector) {
  Vec2 v(3.0, 4.0);
  Vec2 onto(1.0, 0.0);
  Vec2 proj = physics::project_onto_vector(v, onto);
  EXPECT_NEAR(proj.x(), 3.0, 1e-9);
  EXPECT_NEAR(proj.y(), 0.0, 1e-9);
}

// Vector rejection should be orthogonal to onto vector
TEST(VectorOps, RejectIsOrthogonal) {
  Vec2 v(3.0, 4.0);
  Vec2 onto(1.0, 0.0);
  Vec2 rej = physics::reject_from_vector(v, onto);
  EXPECT_NEAR(rej.x(), 0.0, 1e-9);
  EXPECT_NEAR(rej.y(), 4.0, 1e-9);
}

// Cross product of orthogonal unit vectors
TEST(VectorOps, Cross2Basic) {
  Vec2 a(1.0, 0.0);
  Vec2 b(0.0, 1.0);
  EXPECT_DOUBLE_EQ(physics::cross2(a, b), 1.0);
  EXPECT_DOUBLE_EQ(physics::cross2(b, a), -1.0);
}

// Cross product of parallel vectors
TEST(VectorOps, Cross2Parallel) {
  Vec2 a(2.0, 2.0);
  Vec2 b(4.0, 4.0);
  EXPECT_DOUBLE_EQ(physics::cross2(a, b), 0.0);
}

// Centripetal force is zero at zero speed
TEST(Centripetal, ZeroSpeedGivesZeroForce) {
  Vec2 normal(0.0, 1.0);
  Vec2 force = physics::centripetal_force(1500.0, 0.0, 0.01, normal);
  EXPECT_DOUBLE_EQ(force.x(), 0.0);
  EXPECT_DOUBLE_EQ(force.y(), 0.0);
}

// Centripetal force scales with v^2
TEST(Centripetal, ScalesWithSpeedSquared) {
  Vec2 normal(0.0, 1.0);
  double curvature = 0.01;
  Vec2 force_20 = physics::centripetal_force(1500.0, 20.0, curvature, normal);
  Vec2 force_40 = physics::centripetal_force(1500.0, 40.0, curvature, normal);
  EXPECT_NEAR(force_40.y() / (force_20.y() + 1e-12), 4.0, 0.01);
}

// Centrifugal is opposite sign to centripetal
TEST(Centripetal, CentrifugalOppositeSign) {
  Vec2 normal(0.0, 1.0);
  Vec2 centrip = physics::centripetal_force(1500.0, 30.0, 0.01, normal);
  Vec2 centrif = physics::centrifugal_force(1500.0, 30.0, 0.01, normal);
  EXPECT_NEAR(centrif.x(), -centrip.x(), 1e-9);
  EXPECT_NEAR(centrif.y(), -centrip.y(), 1e-9);
}

// VehicleParams defaults should be sensible
TEST(VehicleParams, DefaultsAreSensible) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.mass, 0.0);
  EXPECT_GT(params.wheelbase, 0.0);
  EXPECT_GT(params.max_power, 0.0);
  EXPECT_GT(params.max_torque, 0.0);
  EXPECT_GT(params.max_rpm, params.idle_rpm);
  EXPECT_GT(params.gear_ratios.size(), 0u);
}

// Default gear ratios should be descending (lower gear = higher ratio)
TEST(VehicleParams, GearRatiosDescending) {
  vehicle::VehicleParams params;
  for (size_t i = 1; i < params.gear_ratios.size(); ++i) {
    EXPECT_GT(params.gear_ratios[i - 1], params.gear_ratios[i]);
  }
}

// Tire mu should be positive
TEST(VehicleParams, TireMuPositive) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.tire_mu, 0.0);
  EXPECT_LT(params.tire_mu, 3.0);
}

// Ride height should be small
TEST(VehicleParams, RideHeightReasonable) {
  vehicle::VehicleParams params;
  EXPECT_GT(params.ride_height, 0.0);
  EXPECT_LT(params.ride_height, 0.5);
}
