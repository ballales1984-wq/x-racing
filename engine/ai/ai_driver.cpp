#include "ai/ai_driver.h"
#include <algorithm>
#include <cmath>

namespace p0::ai {

static AIDriverParams difficulty_preset(AIDifficulty diff) {
  AIDriverParams p;
  switch (diff) {
    case AIDifficulty::EASY:
      p.difficulty = AIDifficulty::EASY;
      p.look_ahead_distance = 25.0;
      p.steering_gain = 0.7;
      p.speed_error_gain = 0.3;
      p.max_throttle = 0.85;
      p.max_brake = 0.7;
      p.reaction_delay = 0.15;
      p.error_amplitude = 0.08;
      p.corner_entry_speed_factor = 0.75;
      p.gear_shift_rpm_up = 6500.0;
      p.gear_shift_rpm_down = 2200.0;
      p.enable_defense = false;
      break;
    case AIDifficulty::MEDIUM:
      p.difficulty = AIDifficulty::MEDIUM;
      p.look_ahead_distance = 35.0;
      p.steering_gain = 0.9;
      p.speed_error_gain = 0.45;
      p.max_throttle = 0.95;
      p.max_brake = 0.9;
      p.reaction_delay = 0.05;
      p.error_amplitude = 0.03;
      p.corner_entry_speed_factor = 0.82;
      p.gear_shift_rpm_up = 6800.0;
      p.gear_shift_rpm_down = 2500.0;
      p.enable_defense = false;
      break;
    case AIDifficulty::HARD:
      p.difficulty = AIDifficulty::HARD;
      p.look_ahead_distance = 45.0;
      p.steering_gain = 1.0;
      p.speed_error_gain = 0.55;
      p.max_throttle = 1.0;
      p.max_brake = 1.0;
      p.reaction_delay = 0.0;
      p.error_amplitude = 0.0;
      p.corner_entry_speed_factor = 0.88;
      p.gear_shift_rpm_up = 7000.0;
      p.gear_shift_rpm_down = 2800.0;
      p.enable_defense = true;
      break;
  }
  return p;
}

AIDriver::AIDriver(const AIDriverParams& params) : params_(params) {
  last_input_ = input::InputState{};
}

void AIDriver::set_track(const track::Track& track) {
  track_ = &track;
  has_target_ = false;
}

void AIDriver::set_difficulty(AIDifficulty difficulty) {
  params_ = difficulty_preset(difficulty);
}

void AIDriver::set_target_speed_factor(double factor) {
  params_.corner_entry_speed_factor = std::clamp(factor, 0.5, 1.0);
}

void AIDriver::update(const vehicle::VehicleState& state, double delta_time) {
  if (!track_) return;

  compute_target(state);
  compute_steering(state);
  compute_throttle_brake(state, delta_time);
  compute_gears(state);
}

void AIDriver::compute_target(const vehicle::VehicleState& state) {
  if (!track_) return;

  double look_ahead = params_.look_ahead_distance + state.speed * 1.5;
  double dist = 0.0;
  double step = 2.0;
  double current_d = state.distance_along_track;
  double track_len = track_->length();

  current_d = std::fmod(current_d, track_len);
  if (current_d < 0.0) current_d += track_len;

  Vec2 best_pos = state.position;
  Vec2 best_dir(1.0, 0.0);
  double best_speed = 80.0;

  for (double d = current_d; dist < look_ahead; d += step) {
    double wrapped = std::fmod(d, track_len);
    if (wrapped < 0.0) wrapped += track_len;

    const auto& tp = track_->at(wrapped);
    double curvature = std::abs(tp.curvature);
    double target_speed = get_track_target_speed(tp.position);

    if (curvature > 0.01) {
      target_speed *= params_.corner_entry_speed_factor;
    }

    if (target_speed > best_speed || dist < 5.0) {
      best_pos = tp.position;
      best_dir = tp.tangent.normalized();
      best_speed = target_speed;
    }

    dist += step;
  }

  if (params_.error_amplitude > 0.0) {
    double error = std::sin(state.distance_along_track * 0.5) * params_.error_amplitude * 5.0;
    Vec2 normal = Vec2(-best_dir.y(), best_dir.x());
    best_pos += normal * error;
  }

  current_target_ = best_pos;
  current_target_speed_ = best_speed;
  has_target_ = true;
}

void AIDriver::compute_steering(const vehicle::VehicleState& state) {
  if (!has_target_) return;

  Vec2 forward(std::cos(state.heading), std::sin(state.heading));
  Vec2 to_target = current_target_ - state.position;
  double lateral = to_target.x() * (-forward.y()) + to_target.y() * forward.x();

  double target_heading = std::atan2(current_target_.y() - state.position.y(),
                                      current_target_.x() - state.position.x());
  double heading_error = target_heading - state.heading;
  while (heading_error > kPi) heading_error -= kTwoPi;
  while (heading_error < -kPi) heading_error += kTwoPi;

  double speed_factor = std::clamp(state.speed / 30.0, 0.3, 1.0);
  double curvature_factor = 1.0;

  if (track_) {
    double d = std::fmod(state.distance_along_track, track_->length());
    if (d < 0.0) d += track_->length();
    const auto& tp = track_->at(d);
    curvature_factor = 1.0 / (1.0 + std::abs(tp.curvature) * 50.0);
    curvature_factor = std::clamp(curvature_factor, 0.3, 1.0);
  }

  double steer = heading_error * params_.steering_gain * curvature_factor / speed_factor;
  steer = std::clamp(steer, -1.0, 1.0);

  if (state.speed < 2.0) {
    steer *= 0.5;
  }

  last_input_.steering = steer;
}

void AIDriver::compute_throttle_brake(const vehicle::VehicleState& state, double delta_time) {
  double speed_error = current_target_speed_ - state.speed;
  double derror = speed_error - prev_speed_error_;
  prev_speed_error_ = speed_error;

  double throttle = 0.0;
  double brake = 0.0;

  if (speed_error > 0.0) {
    throttle = std::clamp(params_.speed_error_gain * speed_error * 0.1, 0.0, params_.max_throttle);
    if (state.speed > current_target_speed_ * 1.1) {
      throttle *= 0.5;
    }
  } else {
    brake = std::clamp(-params_.speed_error_gain * speed_error * 0.15, 0.0, params_.max_brake);
    if (state.speed < 5.0) {
      brake = std::max(brake, 0.3);
    }
  }

  double ahead_curve = 0.0;
  if (track_) {
    double look_d = state.distance_along_track + state.speed * 2.0;
    double track_len = track_->length();
    look_d = std::fmod(look_d, track_len);
    if (look_d < 0.0) look_d += track_len;
    ahead_curve = std::abs(track_->at(look_d).curvature);
  }

  if (ahead_curve > 0.05) {
    double brake_factor = std::clamp(ahead_curve * 15.0, 0.0, 1.0);
    brake = std::max(brake, brake_factor * params_.max_brake);
    throttle *= (1.0 - brake_factor);
  }

  last_input_.throttle = std::clamp(throttle, 0.0, 1.0);
  last_input_.brake = std::clamp(brake, 0.0, 1.0);
}

void AIDriver::compute_gears(const vehicle::VehicleState& state) {
  last_input_.upshift = false;
  last_input_.downshift = false;

  if (state.rpm > params_.gear_shift_rpm_up && state.gear < params_.max_gear) {
    last_input_.upshift = true;
  } else if (state.rpm < params_.gear_shift_rpm_down && state.gear > 1) {
    last_input_.downshift = true;
  }
}

double AIDriver::get_track_target_speed(const Vec2& position) const {
  if (!track_) return 80.0;

  double best_speed = 80.0;
  double min_curvature = 0.0;

  for (double d = 0.0; d < track_->length(); d += 5.0) {
    const auto& tp = track_->at(d);
    double dist = (tp.position - position).norm();
    if (dist < 30.0) {
      double curvature = std::abs(tp.curvature);
      double speed = 120.0 / (1.0 + curvature * 80.0);
      if (curvature > min_curvature) {
        best_speed = speed;
        min_curvature = curvature;
      }
    }
  }

  return std::clamp(best_speed, 30.0, 150.0);
}

double AIDriver::curve_radius(const Vec2& pos, double distance) const {
  if (!track_) return 100.0;

  double d = std::fmod(distance, track_->length());
  if (d < 0.0) d += track_->length();
  const auto& tp = track_->at(d);
  double curvature = std::abs(tp.curvature);
  if (curvature < 0.001) return 100.0;
  return 1.0 / curvature;
}

input::InputState AIDriver::poll() {
  return last_input_;
}

bool AIDriver::is_key_down(int key_code) {
  return false;
}

}
