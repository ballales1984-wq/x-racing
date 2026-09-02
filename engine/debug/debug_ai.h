// Project 0 — AI driver diagnostics (snapshot + trend analysis)
// Namespace: p0::debug
#pragma once

#include "common.h"
#include "ai/ai_driver.h"
#include "input/input.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace p0::debug {

// Frozen snapshot of an AI driver's internal state at a point in time.
// Used for rendering AI debug overlays and comparing against targets.
struct AIDriverSnapshot {
  int car_id = -1;
  double current_speed = 0.0;  // m/s
  double target_speed = 0.0;  // m/s
  double speed_error = 0.0;  // m/s, target - current
  double steer_input = 0.0;  // [-1, 1]
  double throttle_input = 0.0;  // [0, 1]
  double brake_input = 0.0;  // [0, 1]
  int current_gear = 1;
  double lookahead_distance = 30.0;  // m
  double steering_gain = 1.0;
  double speed_error_gain = 0.5;
  double max_throttle = 1.0;
  double max_brake = 1.0;
  double reaction_delay = 0.0;  // s, input lag
  double error_amplitude = 0.0;
  double corner_entry_speed_factor = 0.85;
  double gear_shift_rpm_up = 6800.0;
  double gear_shift_rpm_down = 2500.0;
  int max_gear = 6;
  bool enable_defense = false;
  bool overtake_enabled = true;
  double overtake_aggression = 0.5;
  double traffic_adaptation = 0.7;
  double speed_variance = 0.0;
  double steering_jitter = 0.0;
  double defense_willingness = 0.5;
  double lap_time = 0.0;  // s
  double sector_time = 0.0;  // s
  int difficulty = 1;
};

// Aggregated AI health metrics across all active drivers.
struct AIDiagnostics {
  int active_driver_count = 0;
  std::unordered_map<int, AIDriverSnapshot> driver_info;
  bool any_driver_off_track = false;
  bool any_driver_spinning = false;
  double avg_target_speed = 0.0;
  double avg_speed_error = 0.0;
};

class DebugAI {
 public:
  DebugAI() = default;

  void initialize();
  void shutdown();

  void record_input(int car_id, const input::InputState& input,
                    const ai::AIDriverParams& params);
  void update_analysis();
  void reset();

  const AIDiagnostics& current_diagnostics() const { return diagnostics_; }
  bool has_active_driver() const { return active_car_id_ >= 0; }
  int active_car_id() const { return active_car_id_; }
  void set_active_car_id(int id) { active_car_id_ = id; }

 private:
  AIDiagnostics diagnostics_;
  int active_car_id_ = -1;
  double last_speed_ = 0.0;
  double last_yaw_rate_ = 0.0;
  bool was_on_track_ = true;
  double off_track_timer_ = 0.0;
  double spin_timer_ = 0.0;
};

}
