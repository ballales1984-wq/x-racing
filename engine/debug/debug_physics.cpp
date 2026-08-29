#include "debug/debug_physics.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

namespace p0::debug {

void DebugPhysics::initialize() {
  reset();
}

void DebugPhysics::shutdown() {
  reset();
}

void DebugPhysics::reset() {
  diagnostics_ = PhysicsDiagnostics{};
  peak_slip_angle_frame_ = 0.0;
  peak_slip_ratio_frame_ = 0.0;
  max_load_imbalance_ = 0.0;
  critical_tire_frames_ = 0;
  was_on_track_ = true;
  off_track_accum_ = 0.0;
}

void DebugPhysics::analyze(const p0::vehicle::VehicleState& state, double dt) {
  diagnostics_ = PhysicsDiagnostics{};

  analyze_tire_temps(state);
  analyze_tire_wear(state);
  analyze_suspension(state);
  analyze_aero(state);
  analyze_slip(state);

  if (std::abs(state.slip_angle) > peak_slip_angle_frame_) {
    peak_slip_angle_frame_ = std::abs(state.slip_angle);
  }
  if (std::abs(state.slip_ratio) > peak_slip_ratio_frame_) {
    peak_slip_ratio_frame_ = std::abs(state.slip_ratio);
  }
  diagnostics_.peak_slip_angle = peak_slip_angle_frame_;
  diagnostics_.peak_slip_ratio = peak_slip_ratio_frame_;

  const double fl = state.fl_tire_load;
  const double fr = state.fr_tire_load;
  const double rl = state.rl_tire_load;
  const double rr = state.rr_tire_load;
  const double total = fl + fr + rl + rr;
  if (total > 1e-6) {
    const double front = (fl + fr) / total;
    const double rear = (rl + rr) / total;
    const double ideal_front = 0.5;
    const double imbalance = std::abs(front - ideal_front);
    if (imbalance > max_load_imbalance_) max_load_imbalance_ = imbalance;
    diagnostics_.max_tire_load_imbalance = max_load_imbalance_;
  }

  const bool on_track = !state.in_box_lane;
  diagnostics_.on_track = on_track;
  diagnostics_.in_box_lane = state.in_box_lane;
  diagnostics_.lateral_position = 0.0;
  diagnostics_.distance_to_barrier = 0.0;

  if (!on_track && was_on_track_) {
    off_track_accum_ = 0.0;
  }
  if (!on_track) {
    off_track_accum_ += dt;
    if (off_track_accum_ > 2.0) {
      diagnostics_.collision_detected = true;
    }
  }
  was_on_track_ = on_track;

  if (critical_tire_frames_ > 120) {
    diagnostics_.tire_temp_critical = true;
  }

  diagnostics_.tire_status = classify_tire_status(state);
  diagnostics_.suspension_status = classify_suspension_status(state);
  diagnostics_.aero_status = classify_aero_status(state);
}

void DebugPhysics::analyze_tire_temps(const p0::vehicle::VehicleState& state) {
  const double front = state.front_tire_temp;
  const double rear = state.rear_tire_temp;
  const bool front_cold = front < 300.0;
  const bool rear_cold = rear < 300.0;
  const bool front_hot = front > 370.0;
  const bool rear_hot = rear > 370.0;

  if (front_cold || rear_cold) critical_tire_frames_++;
  else critical_tire_frames_ = std::max(0, critical_tire_frames_ - 1);

  if (front_hot || rear_hot) {
    diagnostics_.tire_temp_critical = true;
  }
}

void DebugPhysics::analyze_tire_wear(const p0::vehicle::VehicleState& state) {
  const double front_wear = 1.0 - state.front_tire_wear;
  const double rear_wear = 1.0 - state.rear_tire_wear;
  diagnostics_.tire_wear_critical = front_wear > 0.6 || rear_wear > 0.6;
}

void DebugPhysics::analyze_suspension(const p0::vehicle::VehicleState& state) {
  (void)state;
}

void DebugPhysics::analyze_aero(const p0::vehicle::VehicleState& state) {
  (void)state;
}

void DebugPhysics::analyze_slip(const p0::vehicle::VehicleState& state) {
  diagnostics_.excessive_slip = std::abs(state.slip_angle) > 0.15 ||
                                std::abs(state.slip_ratio) > 0.2;
  if (diagnostics_.excessive_slip) {
    diagnostics_.spin_detected = std::abs(state.yaw_rate) > 3.0 && state.speed > 5.0;
  }
}

std::string DebugPhysics::classify_tire_status(const p0::vehicle::VehicleState& state) const {
  const double front = state.front_tire_temp;
  const double rear = state.rear_tire_temp;
  if (front < 300.0 && rear < 300.0) return "cold";
  if (front > 370.0 || rear > 370.0) return "overheating";
  if (front > 330.0 && front < 360.0 && rear > 330.0 && rear < 360.0) return "optimal";
  return "warming";
}

std::string DebugPhysics::classify_suspension_status(const p0::vehicle::VehicleState& state) const {
  const double roll = std::abs(state.body_roll);
  if (roll > 0.1) return "heavy_roll";
  if (roll > 0.05) return "moderate_roll";
  return "stable";
}

std::string DebugPhysics::classify_aero_status(const p0::vehicle::VehicleState& state) const {
  const double speed = state.speed;
  if (speed < 10.0) return "minimal";
  if (state.aero_downforce > 2000.0) return "high_downforce";
  return "normal";
}

}
