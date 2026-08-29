#include <gtest/gtest.h>
#include "physics/types.h"
#include "physics/tire_model.h"
#include "vehicle/vehicle.h"

using namespace p0::physics;

TEST(RelaxSlipAngle, ZeroVelocityReturnsKinematic) {
  double alpha_k = 0.1;
  double alpha_f = 0.0;
  double r = relax_slip_angle(alpha_k, alpha_f, 0.0, 0.5, 0.01);
  EXPECT_DOUBLE_EQ(r, alpha_k);
}

TEST(RelaxSlipAngle, ZeroRelaxationReturnsKinematic) {
  double alpha_k = 0.1;
  double alpha_f = 0.05;
  double r = relax_slip_angle(alpha_k, alpha_f, 10.0, 0.0, 0.01);
  EXPECT_DOUBLE_EQ(r, alpha_k);
}

TEST(RelaxSlipAngle, ConvergesTowardKinematic) {
  double alpha_k = 0.2;
  double alpha_f = 0.0;
  double v = 20.0;
  double tau = 0.5;
  double dt = 0.01;
  double r = relax_slip_angle(alpha_k, alpha_f, v, tau, dt);
  EXPECT_GT(r, alpha_f);
  EXPECT_LT(r, alpha_k);
}

TEST(CamberLateralForce, LinearWithCamberAndLoad) {
  double fz = 4000.0;
  double camber = 0.02;
  double gain = 1.0;
  double f = camber_lateral_force(camber, fz, gain);
  EXPECT_DOUBLE_EQ(f, gain * camber * fz);
}

TEST(CamberLateralForce, ZeroCamberGivesZero) {
  double f = camber_lateral_force(0.0, 4000.0, 1.0);
  EXPECT_DOUBLE_EQ(f, 0.0);
}

TEST(EffectiveTireLoad, ReducesAtHighLoad) {
  double fz = 8000.0;
  double sensitivity = 0.06;
  double max_ref = 5000.0;
  double eff = effective_tire_load(fz, sensitivity, max_ref);
  EXPECT_LT(eff, fz);
}

TEST(EffectiveTireLoad, ZeroSensitivityPreservesLoad) {
  double eff = effective_tire_load(5000.0, 0.0, 5000.0);
  EXPECT_DOUBLE_EQ(eff, 5000.0);
}

TEST(EffectiveTireLoad, ClampedAtTwoTimesReference) {
  double eff = effective_tire_load(20000.0, 0.1, 5000.0);
  EXPECT_GT(eff, 0.0);
}

TEST(CombinedSlipForce, PureLongitudinal) {
  double fx = 0.0;
  double fy = 0.0;
  combined_slip_force(0.2, 0.0, 4000.0, 1.2, 1.2, 11.0, 11.0, 1.9, 0.97, fx, fy);
  EXPECT_GT(fx, 0.0);
  EXPECT_DOUBLE_EQ(fy, 0.0);
}

TEST(CombinedSlipForce, PureLateral) {
  double fx = 0.0;
  double fy = 0.0;
  combined_slip_force(0.0, 0.15, 4000.0, 1.2, 1.2, 11.0, 11.0, 1.9, 0.97, fx, fy);
  EXPECT_DOUBLE_EQ(fx, 0.0);
  EXPECT_GT(fy, 0.0);
}

TEST(CombinedSlipForce, FrictionEllipseScalesDown) {
  double fx = 0.0;
  double fy = 0.0;
  combined_slip_force(0.9, 0.9, 4000.0, 1.2, 1.2, 11.0, 11.0, 1.9, 0.97, fx, fy);
  double combined = std::sqrt(fx * fx + fy * fy);
  double max_possible = 1.2 * 4000.0 * 1.2;
  EXPECT_LE(combined, max_possible);
}

TEST(LongitudinalSlipRatio, ZeroWheelSpeedDriving) {
  double kappa = longitudinal_slip_ratio(0.0, 0.33, 20.0, 0.0, 0.0, 0.01);
  EXPECT_DOUBLE_EQ(kappa, -20.0 / 20.0);
}

TEST(LongitudinalSlipRatio, WheelSpeedMatchingVehicle) {
  double wheel_omega = 60.0;
  double radius = 0.33;
  double vx = wheel_omega * radius;
  double kappa = longitudinal_slip_ratio(wheel_omega, radius, vx, 0.0, 0.0, 0.01);
  EXPECT_NEAR(kappa, 0.0, 1e-9);
}

TEST(ComputeTireForces, IncludesCamberThrust) {
  double fx = 0.0;
  double fy = 0.0;
  compute_tire_forces(0.0, 0.1, 4000.0, 0.02, 1.0, 1.2, 1.2, 11.0, 11.0, 1.9, 0.97, 0.06, 5000.0, fx, fy);
  EXPECT_GT(fy, 0.0);
}

TEST(ComputeTireForces, LoadSensitivityReducesForce) {
  double fx_low = 0.0;
  double fy_low = 0.0;
  compute_tire_forces(0.1, 0.1, 3000.0, 0.0, 0.0, 1.2, 1.2, 11.0, 11.0, 1.9, 0.97, 0.06, 5000.0, fx_low, fy_low);

  double fx_high = 0.0;
  double fy_high = 0.0;
  compute_tire_forces(0.1, 0.1, 8000.0, 0.0, 0.0, 1.2, 1.2, 11.0, 11.0, 1.9, 0.97, 0.06, 5000.0, fx_high, fy_high);

  double ratio_low = std::sqrt(fx_low * fx_low + fy_low * fy_low) / 3000.0;
  double ratio_high = std::sqrt(fx_high * fx_high + fy_high * fy_high) / 8000.0;
  EXPECT_GT(ratio_low, ratio_high);
}
