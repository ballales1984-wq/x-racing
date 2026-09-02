#include "ai/ai_driver.h"
#include <algorithm>
#include <cmath>

namespace p0::ai {

// ---------------------------------------------------------------------------
//  Difficulty presets
// ---------------------------------------------------------------------------

//! @brief Returns AI driver parameters tuned for the specified difficulty level.
//! @param diff The difficulty tier (EASY, MEDIUM, HARD).
//! @return AIDriverParams configured for the given difficulty.
AIDriverParams AIDriver::difficulty_preset(AIDifficulty diff) {
  AIDriverParams p;
  switch (diff) {
    case AIDifficulty::EASY:
      // Novice-level: slow reactions, makes frequent mistakes, timid racing.
      p.difficulty = AIDifficulty::EASY;
      p.look_ahead_distance = 25.0;
      p.steering_gain = 0.7;
      p.speed_error_gain = 0.3;
      p.max_throttle = 0.85;
      p.max_brake = 0.7;
      p.reaction_delay = 0.15;
      p.error_amplitude = 0.08;
      p.speed_variance = 0.05;
      p.steering_jitter = 0.03;
      p.corner_entry_speed_factor = 0.75;
      p.gear_shift_rpm_up = 6500.0;
      p.gear_shift_rpm_down = 2200.0;
      p.enable_defense = false;
      p.overtake_enabled = false;   // easy AI doesn't attempt passes
      p.overtake_aggression = 0.3;
      p.traffic_adaptation = 0.3;
      p.defense_willingness = 0.2;
      break;

    case AIDifficulty::MEDIUM:
      // Balanced human-like driver.
      p.difficulty = AIDifficulty::MEDIUM;
      p.look_ahead_distance = 35.0;
      p.steering_gain = 0.9;
      p.speed_error_gain = 0.45;
      p.max_throttle = 0.95;
      p.max_brake = 0.9;
      p.reaction_delay = 0.05;
      p.error_amplitude = 0.03;
      p.speed_variance = 0.02;
      p.steering_jitter = 0.01;
      p.corner_entry_speed_factor = 0.82;
      p.gear_shift_rpm_up = 6800.0;
      p.gear_shift_rpm_down = 2500.0;
      p.enable_defense = true;
      p.overtake_enabled = true;
      p.overtake_aggression = 0.5;
      p.traffic_adaptation = 0.7;
      p.defense_willingness = 0.5;
      break;

    case AIDifficulty::HARD:
      // Expert-level: minimal errors, aggressive racing line.
      p.difficulty = AIDifficulty::HARD;
      p.look_ahead_distance = 45.0;
      p.steering_gain = 1.0;
      p.speed_error_gain = 0.55;
      p.max_throttle = 1.0;
      p.max_brake = 1.0;
      p.reaction_delay = 0.0;
      p.error_amplitude = 0.0;
      p.speed_variance = 0.0;
      p.steering_jitter = 0.0;
      p.corner_entry_speed_factor = 0.88;
      p.gear_shift_rpm_up = 7000.0;
      p.gear_shift_rpm_down = 2800.0;
      p.enable_defense = true;
      p.overtake_enabled = true;
      p.overtake_aggression = 0.8;
      p.traffic_adaptation = 0.9;
      p.defense_willingness = 0.8;
      break;
  }
  return p;
}

// ---------------------------------------------------------------------------
//  Construction & configuration
// ---------------------------------------------------------------------------

//! @brief Constructs an AI driver with the given parameters.
//! @param params The AI driver configuration.
AIDriver::AIDriver(const AIDriverParams& params) : params_(params) {
  last_input_ = input::InputState{};

  // Seed the RNG from a non-deterministic source so that each driver
  // instance has unique random behavior.  Default params have zero
  // variance, so deterministic tests are unaffected.
  std::random_device rd;
  rng_.seed(rd());
}

//! @brief Sets the track reference for path planning.
//! @param track The track data.
void AIDriver::set_track(const track::Track& track) {
  track_ = &track;
  has_target_ = false;
}

//! @brief Sets the precomputed racing line samples.
//! @param samples Vector of racing line samples.
void AIDriver::set_racing_line(const std::vector<track::RacingLineSample>& samples) {
  racing_line_ = samples;
  use_racing_line_ = !samples.empty();
}

//! @brief Re-applies a difficulty preset, resetting all parameters.
//! @param difficulty The new difficulty level.
void AIDriver::set_difficulty(AIDifficulty difficulty) {
  params_ = difficulty_preset(difficulty);
}

//! @brief Sets the corner-entry speed factor (clamped to 0.5-1.0).
//! @param factor The speed factor as a fraction of optimal.
void AIDriver::set_target_speed_factor(double factor) {
  // Clamp to a sane range: 50 % to 100 % of the pre-computed corner speed.
  params_.corner_entry_speed_factor = std::clamp(factor, 0.5, 1.0);
}

//! @brief Sets the list of nearby cars for traffic-aware decisions.
//! @param cars Vector of vehicle states for other cars on track.
void AIDriver::set_nearby_cars(const std::vector<vehicle::VehicleState>& cars) {
  nearby_cars_ = cars;
}

// ---------------------------------------------------------------------------
//  Update pipeline
// ---------------------------------------------------------------------------

//! @brief Main update entry point. Computes all control inputs for this tick.
//!        Runs traffic detection, path planning, and control computation.
//! @param state Current vehicle state.
//! @param delta_time Time elapsed since last update in seconds.
void AIDriver::update(const vehicle::VehicleState& state, double delta_time) {
  if (!track_) return;

  // Save the previous output before recomputing — needed for
  // reaction-delay blending in the final phase.
  input::InputState previous_output = last_input_;

  // Phase 1: traffic awareness.
  // Inspect nearby cars and update stuck_behind_timer_ / overtake_urgency_.
  // Must run before compute_target so urgency can influence path selection.
  detect_traffic(state, delta_time);

  // Phase 2: path planning — compute the target position and target speed.
  // Uses overtake_urgency_ to modulate tactical line changes.
  compute_target(state);

  // Phase 3: convert the target into raw control commands.
  compute_steering(state);
  compute_throttle_brake(state, delta_time);
  compute_gears(state);

  // Phase 4: human-error simulation.
  // Blend the fresh computation with the previous output to simulate a
  // human-like reaction delay, then add random imperfections.
  apply_reaction_delay(last_input_, previous_output, delta_time);
  apply_human_errors(last_input_);
}

// ---------------------------------------------------------------------------
//  Racing-line lookup
// ---------------------------------------------------------------------------

//! @brief Finds the nearest racing-line sample to the given position.
//! @param position The world position to search from.
//! @return Pointer to the nearest sample, or nullptr if no racing line is set.
const track::RacingLineSample* AIDriver::lookup_racing_line(const Vec2& position) const {
  if (!use_racing_line_ || racing_line_.empty()) return nullptr;

  // Linear scan (racing lines are typically <2000 samples, so this is fine).
  const track::RacingLineSample* best = nullptr;
  double best_dist = 1e9;

  for (const auto& sample : racing_line_) {
    double d = (sample.transform.position - position).norm();
    if (d < best_dist) {
      best_dist = d;
      best = &sample;
    }
  }

  return best;
}

// ---------------------------------------------------------------------------
//  Target computation
// ---------------------------------------------------------------------------

//! @brief Computes the target position and speed for the AI to follow.
//!        Uses racing line if available, otherwise samples track geometry.
//! @param state Current vehicle state.
void AIDriver::compute_target(const vehicle::VehicleState& state) {
  if (!track_) return;

  // ---- Option A: use the pre-computed optimal racing line ----
  if (use_racing_line_) {
    const track::RacingLineSample* rl = lookup_racing_line(state.position);
    if (rl) {
      current_target_ = rl->transform.position;
      // Apply the corner-entry speed factor to the racing-line speed.
      current_target_speed_ = rl->speed_m_s * params_.corner_entry_speed_factor;
      has_target_ = true;

      // If no tactical adjustments are active, we can return early.
      if (nearby_cars_.empty() ||
          (!params_.overtake_enabled && !params_.enable_defense)) {
        return;
      }
    }
  }

  // ---- Option B: sample the track ahead and pick a lookahead point ----
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
      double target_speed = get_track_target_speed(tp.position, d);

    // Apply corner-entry speed factor in curves.
    if (curvature > 0.01) {
      target_speed *= params_.corner_entry_speed_factor;
    }

    // Prefer points that are either farther ahead or require lower speed
    // (i.e. the tightest corner in the lookahead window).
    if (target_speed > best_speed || dist < 5.0) {
      best_pos = tp.position;
      best_dir = tp.tangent.normalized();
      best_speed = target_speed;
    }

    dist += step;
  }

  // ---- Apply human-like path imperfection (error_amplitude) ----
  // Introduces a sinusoidal lateral deviation that simulates the imperfect
  // racing line a human driver might take.
  if (params_.error_amplitude > 0.0) {
    double error = std::sin(state.distance_along_track * 0.5)
                   * params_.error_amplitude * 5.0;
    Vec2 normal(-best_dir.y(), best_dir.x());
    best_pos += normal * error;
  }

  // ---- Look up the current track curvature for tactical decisions ----
  double track_curvature = 0.0;
  {
    double d = std::fmod(state.distance_along_track, track_len);
    if (d < 0.0) d += track_len;
    track_curvature = track_->at(d).curvature;
  }

  // ---- Apply overtaking and defensive adjustments ----
  // Overtaking moves the target to the side of a slower rival on straights.
  // Defense claims the racing line against an attacker in a corner.
  best_pos = adjust_for_overtaking(best_pos, state, track_curvature);
  best_pos = adjust_for_defense(best_pos, state, track_curvature);

  current_target_ = best_pos;
  current_target_speed_ = best_speed;
  has_target_ = true;
}

// ---------------------------------------------------------------------------
//  Steering computation
// ---------------------------------------------------------------------------

//! @brief Computes steering input based on heading error to the target.
//!        Reduces steering authority at high speed and in sharp curves.
//! @param state Current vehicle state.
void AIDriver::compute_steering(const vehicle::VehicleState& state) {
  if (!has_target_) return;

  // Forward and right vectors from the vehicle's heading.
  Vec2 forward(std::cos(state.heading), std::sin(state.heading));
  Vec2 to_target = current_target_ - state.position;

  // Lateral error: projection of (target - position) onto the right axis.
  double lateral = to_target.x() * (-forward.y()) + to_target.y() * forward.x();

  // Heading error: angle between current heading and direction to target.
  double target_heading = std::atan2(current_target_.y() - state.position.y(),
                                     current_target_.x() - state.position.x());
  double heading_error = target_heading - state.heading;
  while (heading_error > kPi) heading_error -= kTwoPi;
  while (heading_error < -kPi) heading_error += kTwoPi;

  // Reduce steering authority at high speed and in sharp curves.
  double speed_factor = std::clamp(state.speed / 30.0, 0.3, 1.0);
  double curvature_factor = 1.0;

  if (track_) {
    double d = std::fmod(state.distance_along_track, track_->length());
    if (d < 0.0) d += track_->length();
    const auto& tp = track_->at(d);
    // In tighter curves, reduce steering to avoid over-correction.
    curvature_factor = 1.0 / (1.0 + std::abs(tp.curvature) * 50.0);
    curvature_factor = std::clamp(curvature_factor, 0.3, 1.0);
  }

  double steer = heading_error * params_.steering_gain * curvature_factor / speed_factor;
  steer = std::clamp(steer, -1.0, 1.0);

  // At very low speed the kinematic bicycle model is poorly approximated by
  // heading error alone: the car has lots of authority but no centrifugal load
  // to settle the heading. Apply a small extra gain to compensate, while
  // still clamping to [-1, +1] above.
  if (state.speed < 2.0) {
    steer *= 1.25;
    steer = std::clamp(steer, -1.0, 1.0);
  }

  last_input_.steering = steer;
}

// ---------------------------------------------------------------------------
//  Throttle & brake computation
// ---------------------------------------------------------------------------

//! @brief Computes throttle and brake based on speed error.
//!        Includes corner anticipation braking.
//! @param state Current vehicle state.
//! @param delta_time Time elapsed since last update.
void AIDriver::compute_throttle_brake(const vehicle::VehicleState& state, double delta_time) {
  // Start from the racing-line target speed, then reduce for any car
  // that is too close ahead (collision avoidance).
  double target_speed = current_target_speed_;
  target_speed -= traffic_speed_adjustment(state);

  // PID-like speed controller.
  double speed_error = target_speed - state.speed;
  prev_speed_error_ = speed_error;

  double throttle = 0.0;
  double brake = 0.0;

  // Phase 1: basic speed matching.
  if (speed_error > 0.0) {
    throttle = std::clamp(params_.speed_error_gain * speed_error * 0.1,
                          0.0, params_.max_throttle);
    if (state.speed > target_speed * 1.1) {
      throttle *= 0.5;  // ease off when approaching target
    }
  } else {
    brake = std::clamp(-params_.speed_error_gain * speed_error * 0.15,
                       0.0, params_.max_brake);
    if (state.speed < 5.0) {
      brake = std::max(brake, 0.3);  // rolling brake at standstill
    }
  }

  // Phase 2: corner anticipation — brake before sharp curves ahead.
  // Look a short distance ahead based on current speed.
  double ahead_curve = 0.0;
  if (track_) {
    double look_d = state.distance_along_track + state.speed * 2.0;
    double track_len = track_->length();
    look_d = std::fmod(look_d, track_len);
    if (look_d < 0.0) look_d += track_len;
    ahead_curve = std::abs(track_->at(look_d).curvature);
  }

  // Apply progressive braking as curvature increases.
  if (ahead_curve > 0.05) {
    double brake_factor = std::clamp(ahead_curve * 15.0, 0.0, 1.0);
    brake = std::max(brake, brake_factor * params_.max_brake);
    throttle *= (1.0 - brake_factor);
  }

  last_input_.throttle = std::clamp(throttle, 0.0, 1.0);
  last_input_.brake = std::clamp(brake, 0.0, 1.0);
}

// ---------------------------------------------------------------------------
//  Gear shifting
// ---------------------------------------------------------------------------

//! @brief Determines whether to upshift or downshift based on current RPM.
//! @param state Current vehicle state.
void AIDriver::compute_gears(const vehicle::VehicleState& state) {
  last_input_.upshift = false;
  last_input_.downshift = false;

  // Upshift when RPM is high enough and we're not in the top gear.
  if (state.rpm > params_.gear_shift_rpm_up && state.gear < params_.max_gear) {
    last_input_.upshift = true;
  }
  // Downshift when RPM drops too low and we're above first gear.
  else if (state.rpm < params_.gear_shift_rpm_down && state.gear > 1) {
    last_input_.downshift = true;
  }
}

// ---------------------------------------------------------------------------
//  Track geometry helpers
// ---------------------------------------------------------------------------

//! @brief Returns a target speed for a position based on nearby curvature.
//!        Scans only forward from track_distance to avoid reacting to curves
//!        that have already been passed.
//! @param position The world position to evaluate.
//! @param track_distance Distance along the track centerline at the position.
//! @return Target speed in m/s (30-150).
double AIDriver::get_track_target_speed(const Vec2& position, double track_distance) const {
  if (!track_) return 80.0;

  double best_speed = 80.0;
  double min_curvature = 0.0;
  double track_len = track_->length();

  // Scan 30 m ahead for the tightest curve.
  for (double d = track_distance; d < track_distance + 30.0; d += 5.0) {
    double wrapped = std::fmod(d, track_len);
    if (wrapped < 0.0) wrapped += track_len;

    const auto& tp = track_->at(wrapped);
    double curvature = std::abs(tp.curvature);
    double speed = 120.0 / (1.0 + curvature * 80.0);
    if (curvature > min_curvature) {
      best_speed = speed;
      min_curvature = curvature;
    }
  }

  return std::clamp(best_speed, 30.0, 150.0);
}

//! @brief Returns the curve radius at a given track distance.
//! @param pos The position (unused, kept for API compatibility).
//! @param distance Distance along the track centerline.
//! @return Curve radius in meters.
double AIDriver::curve_radius(const Vec2& pos, double distance) const {
  if (!track_) return 100.0;

  double d = std::fmod(distance, track_->length());
  if (d < 0.0) d += track_->length();
  const auto& tp = track_->at(d);
  double curvature = std::abs(tp.curvature);
  if (curvature < 0.001) return 100.0;
  return 1.0 / curvature;
}

// ---------------------------------------------------------------------------
//  Traffic detection & adaptation
// ---------------------------------------------------------------------------

//! @brief Inspects nearby cars and updates traffic state.
//!        Tracks stuck-behind timer and overtake urgency.
//! @param state Current vehicle state.
//! @param delta_time Time elapsed since last update.
void AIDriver::detect_traffic(const vehicle::VehicleState& state, double delta_time) {
  if (nearby_cars_.empty() || !track_) {
    // No traffic to evaluate — decay overtake urgency back to zero.
    overtake_urgency_ *= std::max(0.0, 1.0 - delta_time * 3.0);
    stuck_behind_timer_ = 0.0;
    return;
  }

  Vec2 forward(std::cos(state.heading), std::sin(state.heading));
  Vec2 right(-forward.y(), forward.x());

  bool car_ahead = false;

  for (const auto& car : nearby_cars_) {
    Vec2 rel = car.position - state.position;
    double long_dist = rel.dot(forward);       // distance along our forward axis
    double lat_dist = std::abs(rel.dot(right)); // lateral offset

    // A car is "directly ahead" if it's within 25 m longitudinally
    // and 4 m laterally.
    if (long_dist > 0.0 && long_dist < 25.0 && lat_dist < 4.0) {
      car_ahead = true;

      // If the car ahead is significantly slower, we are being held up.
      double speed_deficit = current_target_speed_ - car.speed;
      if (speed_deficit > 3.0) {
        stuck_behind_timer_ += delta_time;
      }
    }
  }

  if (car_ahead && stuck_behind_timer_ > 0.5) {
    // Been stuck for more than 0.5 s — build up overtake urgency.
    overtake_urgency_ = std::min(1.0,
      overtake_urgency_ + delta_time * 0.5 * params_.traffic_adaptation);
  } else if (!car_ahead) {
    // Clear of traffic — reset and decay urgency.
    stuck_behind_timer_ = 0.0;
    overtake_urgency_ *= std::max(0.0, 1.0 - delta_time * 2.0);
  }
}

//! @brief Returns a speed reduction for collision avoidance.
//!        Applied when a slower car is directly ahead.
//! @param state Current vehicle state.
//! @return Speed reduction in m/s.
double AIDriver::traffic_speed_adjustment(const vehicle::VehicleState& state) const {
  // When a slower car is directly ahead within the safety envelope,
  // reduce the target speed to avoid a collision.
  if (nearby_cars_.empty()) return 0.0;

  Vec2 forward(std::cos(state.heading), std::sin(state.heading));
  Vec2 right(-forward.y(), forward.x());

  for (const auto& car : nearby_cars_) {
    Vec2 rel = car.position - state.position;
    double long_dist = rel.dot(forward);
    double lateral = std::abs(rel.dot(right));

    // Car directly ahead within 15 m and 3.5 m laterally.
    if (long_dist > 0.0 && long_dist < 15.0 && lateral < 3.5) {
      // Scale the speed reduction by proximity.
      double factor = std::clamp(1.0 - long_dist / 15.0, 0.0, 0.7);
      if (car.speed < current_target_speed_) {
        return std::max(0.0, (current_target_speed_ - car.speed) * factor);
      }
    }
  }

  return 0.0;
}

// ---------------------------------------------------------------------------
//  Overtaking adjustment
// ---------------------------------------------------------------------------

//! @brief Adjusts the target position for overtaking maneuvers.
//!        Only active on straights with slower cars ahead.
//! @param target The original target position.
//! @param state Current vehicle state.
//! @param curvature Current track curvature.
//! @return Adjusted target position.
Vec2 AIDriver::adjust_for_overtaking(const Vec2& target, const vehicle::VehicleState& state, double curvature) {
  // Guard: overtaking only when enabled and cars are present.
  if (!params_.overtake_enabled || nearby_cars_.empty() || !track_) return target;

  // Overtaking requires a straight or gentle curve — sharp corners are
  // too dangerous to attempt a pass.
  if (std::abs(curvature) > 0.03) return target;

  double d = std::fmod(state.distance_along_track, track_->length());
  if (d < 0.0) d += track_->length();
  Vec2 track_normal = track_->at(d).normal;

  Vec2 forward(std::cos(state.heading), std::sin(state.heading));
  Vec2 right(-forward.y(), forward.x());

  Vec2 best_adjust = target;
  double best_score = 0.0;

  for (const auto& car : nearby_cars_) {
    Vec2 rel = car.position - state.position;
    double long_dist = rel.dot(forward);
    double lat_dist = rel.dot(right);

    // Only consider cars ahead, within 20 m, within 5 m laterally.
    if (long_dist <= 0.0 || long_dist > 20.0 || std::abs(lat_dist) > 5.0) continue;

    // Don't overtake if we're not faster than the car ahead.
    if (state.speed < car.speed - 2.0) continue;

    // Score: urgency × proximity × speed advantage.
    double speed_advantage = state.speed - car.speed;
    double prox_score = 1.0 - long_dist / 20.0;
    double score = (params_.overtake_aggression + overtake_urgency_) * 0.5
                 * prox_score
                 * (1.0 + speed_advantage * 0.1);

    if (score > best_score) {
      best_score = score;
      // Move to the side opposite the car ahead: if the rival is on
      // our right, we steer left, and vice-versa.
      double sign = (lat_dist > 0.0) ? 1.0 : -1.0;
      double offset = sign * (2.0 + overtake_urgency_ * 3.0);
      best_adjust = target + track_normal * offset;
    }
  }

  return best_adjust;
}

// ---------------------------------------------------------------------------
//  Defense adjustment
// ---------------------------------------------------------------------------

//! @brief Adjusts the target position to defend against attackers.
//!        Only active in corners with cars alongside.
//! @param target The original target position.
//! @param state Current vehicle state.
//! @param curvature Current track curvature.
//! @return Adjusted target position.
Vec2 AIDriver::adjust_for_defense(const Vec2& target, const vehicle::VehicleState& state, double curvature) {
  // Guard: defense only when enabled and cars are present.
  if (!params_.enable_defense || nearby_cars_.empty() || !track_) return target;

  // Defense is relevant in corners: we move to claim the racing line
  // that an attacker would use to get past.
  if (std::abs(curvature) < 0.01) return target;

  double d = std::fmod(state.distance_along_track, track_->length());
  if (d < 0.0) d += track_->length();
  Vec2 track_normal = track_->at(d).normal;

  Vec2 forward(std::cos(state.heading), std::sin(state.heading));
  Vec2 right(-forward.y(), forward.x());

  Vec2 best_adjust = target;
  double best_gain = 0.0;

  for (const auto& car : nearby_cars_) {
    Vec2 rel = car.position - state.position;
    double long_dist = rel.dot(forward);
    double lat_dist = rel.dot(right);

    // Only defend against cars within 3 m longitudinally (alongside).
    if (std::abs(long_dist) > 3.0) continue;
    if (std::abs(lat_dist) > 5.0) continue;

    double gain = (1.0 - std::abs(lat_dist) / 5.0) * params_.defense_willingness;
    if (gain > best_gain) {
      best_gain = gain;
      // Move toward the attacker to block their line.
      double sign = (lat_dist > 0.0) ? -1.0 : 1.0;
      double offset = sign * gain * 2.5;
      best_adjust = target + track_normal * offset;
    }
  }

  return best_adjust;
}

// ---------------------------------------------------------------------------
//  Human error simulation
// ---------------------------------------------------------------------------

//! @brief Blends current input with previous output to simulate reaction delay.
//! @param out The output input state (modified in place).
//! @param previous The previous frame's input state.
//! @param delta_time Time elapsed since last update.
void AIDriver::apply_reaction_delay(input::InputState& out, const input::InputState& previous, double delta_time) {
  // If no reaction delay is configured, the computed input passes through unchanged.
  if (params_.reaction_delay <= 0.0 || delta_time <= 0.0) return;

  // Blend rate: higher delta_time relative to delay → more of the new value.
  // With delay = 0.1 s and dt = 1/120 s, rate ≈ 0.087 per frame — the output
  // gradually approaches the new target rather than jumping instantly.
  double rate = delta_time / (params_.reaction_delay + delta_time);

  // Blend continuous axes; discrete flags (upshift/downshift) pass through.
  out.steering = p0::lerp(previous.steering, out.steering, rate);
  out.throttle = std::clamp(p0::lerp(previous.throttle, out.throttle, rate), 0.0, 1.0);
  out.brake = std::clamp(p0::lerp(previous.brake, out.brake, rate), 0.0, 1.0);
}

//! @brief Adds random steering jitter and throttle variance for realism.
//! @param out The output input state (modified in place).
void AIDriver::apply_human_errors(input::InputState& out) {
  // Random steering jitter — small noise that mimics hand tremor.
  if (params_.steering_jitter > 0.0) {
    std::uniform_real_distribution<double> jitter(-params_.steering_jitter, params_.steering_jitter);
    out.steering += jitter(rng_);
    out.steering = std::clamp(out.steering, -1.0, 1.0);
  }

  // Random throttle variance — simulates inconsistent pedal pressure.
  if (params_.speed_variance > 0.0) {
    std::uniform_real_distribution<double> var(
      1.0 - params_.speed_variance, 1.0 + params_.speed_variance);
    out.throttle *= var(rng_);
    out.throttle = std::clamp(out.throttle, 0.0, 1.0);
  }
}

// ---------------------------------------------------------------------------
//  Input polling
// ---------------------------------------------------------------------------

//! @brief Returns the last computed input state.
//! @return The current InputState.
input::InputState AIDriver::poll() {
  return last_input_;
}

//! @brief AI driver never generates key events.
//! @param key_code Unused.
//! @return Always false.
bool AIDriver::is_key_down(int key_code) {
  // AI never generates key events.
  (void)key_code;
  return false;
}

}
