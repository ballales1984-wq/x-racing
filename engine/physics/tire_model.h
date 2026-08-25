#pragma once

#include "common.h"
#include "vehicle/vehicle.h"

namespace p0::physics {

// First-order relaxation length for slip angle dynamics.
// alpha_filtered converges toward alpha_kinematic with time constant
// tau = relaxation_length / v_x.
inline double relax_slip_angle(double alpha_kinematic, double alpha_filtered,
                                double v_x, double relaxation_length, double dt) {
  if (v_x < kEpsilon || relaxation_length < kEpsilon) {
    return alpha_kinematic;
  }
  const double omega = v_x / relaxation_length;
  const double k = 1.0 - std::exp(-omega * dt);
  return alpha_filtered + (alpha_kinematic - alpha_filtered) * k;
}

// Camber-induced lateral force (simplified linear model).
inline double camber_lateral_force(double camber_angle, double fz, double camber_gain) {
  return camber_gain * camber_angle * fz;
}

// Tire load sensitivity: effective normal load considering non-linear grip vs load.
inline double effective_tire_load(double fz, double sensitivity, double max_ref_load) {
  const double load_ratio = std::min(fz / max_ref_load, 2.0);
  return fz * (1.0 - sensitivity * load_ratio);
}

// Combined slip tire model using friction ellipse.
//
// When a tire experiences both longitudinal slip (acceleration/braking) and
// lateral slip (cornering), the total friction force is limited by the
// friction ellipse. This prevents unrealistic force combinations.
//
// Input:
//   sigma_x: longitudinal slip ratio [-1, 1]
//   alpha: lateral slip angle [rad]
//   fz: normal load [N]
//   mu_x: longitudinal friction coefficient
//   mu_y: lateral friction coefficient
//   b_long: Pacejka stiffness for longitudinal
//   b_lat: Pacejka stiffness for lateral
//   c: Pacejka shape factor
//   e: Pacejka curvature factor
//
// Output:
//   fx: longitudinal force [N]
//   fy: lateral force [N]
inline void combined_slip_force(double sigma_x, double alpha, double fz,
                                 double mu_x, double mu_y,
                                 double b_long, double b_lat,
                                 double c, double e,
                                 double& fx, double& fy) {
  // Pure slip forces from Pacejka
  const double fx_pure = mu_x * fz * pacejka_tire_model(sigma_x, 1.0, b_long, c, e);
  const double fy_pure = mu_y * fz * pacejka_tire_model(alpha, 1.0, b_lat, c, e);

  // Friction ellipse combination
  // (fx/fx_max)^2 + (fy/fy_max)^2 <= 1
  const double fx_max = std::abs(mu_x * fz);
  const double fy_max = std::abs(mu_y * fz);

  if (fx_max < kEpsilon || fy_max < kEpsilon) {
    fx = 0.0;
    fy = 0.0;
    return;
  }

  // Normalized forces
  const double nx = fx_pure / fx_max;
  const double ny = fy_pure / fy_max;

  // Check if combined slip exceeds friction ellipse
  const double combined = std::sqrt(nx * nx + ny * ny);

  if (combined > 1.0) {
    // Scale down to friction ellipse boundary
    const double scale = 1.0 / combined;
    fx = fx_pure * scale;
    fy = fy_pure * scale;
  } else {
    fx = fx_pure;
    fy = fy_pure;
  }
}

// Calculate longitudinal slip ratio for a driven/braked wheel.
// kappa = (omega_wheel * r - v_x) / max(|v_x|, epsilon)
// Positive = driving slip, Negative = braking slip
inline double longitudinal_slip_ratio(double wheel_angular_speed, double wheel_radius,
                                       double v_x, double engine_torque,
                                       double brake_force, double dt) {
  const double wheel_speed = wheel_angular_speed * wheel_radius;
  const double denom = std::max(std::abs(v_x), 1.0);
  return (wheel_speed - v_x) / denom;
}

// Combined tire force for a single wheel with full state.
// Returns forces in vehicle longitudinal/lateral frame.
inline void compute_tire_forces(double sigma_x, double alpha, double fz,
                                 double camber, double camber_gain,
                                 double mu_x, double mu_y,
                                 double b_long, double b_lat,
                                 double c, double e,
                                 double& fx, double& fy) {
  // Apply load sensitivity
  const double eff_fz = effective_tire_load(fz, 0.08, 4000.0);

  // Combined slip with friction ellipse
  combined_slip_force(sigma_x, alpha, eff_fz, mu_x, mu_y,
                      b_long, b_lat, c, e, fx, fy);

  // Add camber thrust to lateral force
  fy += camber_lateral_force(camber, eff_fz, camber_gain);
}

}
