#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include "input/input.h"
#include "input/input_manager.h"
#include "track/track.h"
#include "track/track_data.h"
#include "ai/racing_line.h"
#include <memory>
#include <functional>

namespace p0::ai {

enum class AIDifficulty : uint8_t {
  EASY = 0,
  MEDIUM,
  HARD
};

struct AIDriverParams {
  AIDifficulty difficulty = AIDifficulty::MEDIUM;
  double look_ahead_distance = 30.0;
  double steering_gain = 1.0;
  double speed_error_gain = 0.5;
  double max_throttle = 1.0;
  double max_brake = 1.0;
  double reaction_delay = 0.0;
  double error_amplitude = 0.0;
  double corner_entry_speed_factor = 0.85;
  double gear_shift_rpm_up = 6800.0;
  double gear_shift_rpm_down = 2500.0;
  int max_gear = 6;
  bool enable_defense = false;
  double overtake_aggression = 0.5;
  double defense_willingness = 0.5;
};

class AIDriver : public input::InputManager {
 public:
  explicit AIDriver(const AIDriverParams& params = {});
  ~AIDriver() override = default;

  static AIDriverParams difficulty_preset(AIDifficulty diff);

  void set_track(const track::Track& track);
  void set_racing_line(const std::vector<track::RacingLineSample>& samples);
  void update(const vehicle::VehicleState& state, double delta_time);
  void set_difficulty(AIDifficulty difficulty);
  void set_target_speed_factor(double factor);
  void set_nearby_cars(const std::vector<vehicle::VehicleState>& cars);

  input::InputState poll() override;
  bool is_key_down(int key_code) override;

  const input::InputState& last_input() const { return last_input_; }
  const AIDriverParams& params() const { return params_; }

 private:
  AIDriverParams params_;
  input::InputState last_input_;
  const track::Track* track_ = nullptr;
  std::vector<track::RacingLineSample> racing_line_;
  bool use_racing_line_ = false;
  Vec2 current_target_;
  double current_target_speed_ = 0.0;
  double steering_integral_ = 0.0;
  double prev_speed_error_ = 0.0;
  double delay_timer_ = 0.0;
  bool has_target_ = false;
  std::vector<vehicle::VehicleState> nearby_cars_;

  void compute_target(const vehicle::VehicleState& state);
  void compute_steering(const vehicle::VehicleState& state);
  void compute_throttle_brake(const vehicle::VehicleState& state, double delta_time);
  void compute_gears(const vehicle::VehicleState& state);
  double get_track_target_speed(const Vec2& position) const;
  double curve_radius(const Vec2& pos, double distance) const;
  const track::RacingLineSample* lookup_racing_line(const Vec2& position) const;
  Vec2 adjust_for_overtaking(const Vec2& target, const vehicle::VehicleState& state, double curvature);
  Vec2 adjust_for_defense(const Vec2& target, const vehicle::VehicleState& state, double curvature);
};

}
