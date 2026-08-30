#include "debug/debug_ai.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace p0::debug {

void DebugAI::initialize() {
  reset();
}

void DebugAI::shutdown() {
  reset();
}

void DebugAI::reset() {
  diagnostics_ = AIDiagnostics{};
  active_car_id_ = -1;
  last_speed_ = 0.0;
  last_yaw_rate_ = 0.0;
  was_on_track_ = true;
  off_track_timer_ = 0.0;
  spin_timer_ = 0.0;
}

void DebugAI::record_input(int car_id, const input::InputState& input,
                           const ai::AIDriverParams& params) {
  AIDriverSnapshot snap;
  snap.car_id = car_id;
  snap.current_speed = last_speed_;
  snap.steer_input = input.steering;
  snap.throttle_input = input.throttle;
  snap.brake_input = input.brake;
  snap.current_gear = 0;
  snap.lookahead_distance = params.look_ahead_distance;
  snap.steering_gain = params.steering_gain;
  snap.speed_error_gain = params.speed_error_gain;
  snap.max_throttle = params.max_throttle;
  snap.max_brake = params.max_brake;
  snap.reaction_delay = params.reaction_delay;
  snap.error_amplitude = params.error_amplitude;
  snap.corner_entry_speed_factor = params.corner_entry_speed_factor;
  snap.gear_shift_rpm_up = params.gear_shift_rpm_up;
  snap.gear_shift_rpm_down = params.gear_shift_rpm_down;
  snap.max_gear = params.max_gear;
  snap.enable_defense = params.enable_defense;
  snap.overtake_enabled = params.overtake_enabled;
  snap.overtake_aggression = params.overtake_aggression;
  snap.traffic_adaptation = params.traffic_adaptation;
  snap.speed_variance = params.speed_variance;
  snap.steering_jitter = params.steering_jitter;
  snap.defense_willingness = params.defense_willingness;
  snap.difficulty = static_cast<int>(params.difficulty);

  diagnostics_.driver_info[car_id] = snap;
  if (active_car_id_ < 0) active_car_id_ = car_id;
  diagnostics_.active_driver_count = static_cast<int>(diagnostics_.driver_info.size());
}

void DebugAI::update_analysis() {
  double total_target_speed = 0.0;
  double total_speed_error = 0.0;
  int count = 0;

  for (auto& [car_id, snap] : diagnostics_.driver_info) {
    snap.speed_error = snap.target_speed - snap.current_speed;
    if (snap.target_speed > 0.01) {
      total_target_speed += snap.target_speed;
      total_speed_error += std::abs(snap.speed_error);
      count++;
    }
  }

  if (count > 0) {
    diagnostics_.avg_target_speed = total_target_speed / count;
    diagnostics_.avg_speed_error = total_speed_error / count;
  }

  bool any_off = false;
  bool any_spin = false;
  for (const auto& [car_id, snap] : diagnostics_.driver_info) {
    if (snap.current_speed < 2.0 && snap.target_speed > 10.0) {
      any_off = true;
    }
    if (std::abs(snap.steer_input) > 0.9 && snap.current_speed > 5.0) {
      any_spin = true;
    }
  }
  diagnostics_.any_driver_off_track = any_off;
  diagnostics_.any_driver_spinning = any_spin;
}

}
