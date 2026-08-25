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

}
