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
#include <random>

// Project 0 — AI driver subsystem
// Namespace: p0::ai
namespace p0::ai {

// Difficulty tiers that map to progressively more skilled AI profiles.
//   EASY   — high imperfection, slow reactions, timid racing
//   MEDIUM — balanced, human-like mistakes
//   HARD   — near-optimal, minimal errors, aggressive racing
enum class AIDifficulty : uint8_t {
  EASY = 0,
  MEDIUM,
  HARD
};

// Tunable parameters for a single AI driver instance.
// Populated via difficulty_preset() or configured manually.
struct AIDriverParams {
  AIDifficulty difficulty = AIDifficulty::MEDIUM;

  // --- Path planning -------------------------------------------------------
  double look_ahead_distance = 30.0;        // m — how far ahead to look for targets
  double steering_gain = 1.0;              // proportional gain for heading correction
  double speed_error_gain = 0.5;            // proportional gain for throttle/brake PID
  double corner_entry_speed_factor = 0.85;  // fraction of racing-line speed to target

  // --- Powertrain ----------------------------------------------------------
  double max_throttle = 1.0;               // ceiling for throttle output [0,1]
  double max_brake = 1.0;                  // ceiling for brake output [0,1]
  double gear_shift_rpm_up = 6800.0;       // RPM threshold for upshifting
  double gear_shift_rpm_down = 2500.0;     // RPM threshold for downshifting
  int    max_gear = 6;                     // maximum usable gear (1-based)

  // --- Human error simulation ---------------------------------------------
  double reaction_delay = 0.0;             // s — lag before reacting to a new target
  double error_amplitude = 0.0;            // [0,1] lateral path deviation magnitude
  double speed_variance = 0.0;             // [0,1] random throttle variation for realism
  double steering_jitter = 0.0;            // [0,1] random steering noise

  // --- Overtaking ----------------------------------------------------------
  bool   overtake_enabled = true;          // master switch for overtaking behavior
  double overtake_aggression = 0.5;        // [0,1] eagerness to attempt a pass
  double traffic_adaptation = 0.7;         // [0,1] sensitivity to being stuck behind

  // --- Defense -------------------------------------------------------------
  bool   enable_defense = false;           // allow defending position against rivals
  double defense_willingness = 0.5;       // [0,1] willingness to move to block
};

// AI driver that produces InputState from vehicle state and track geometry.
// Inherits from InputManager so it can be used as a drop-in replacement
// for human input in the simulation loop.
class AIDriver : public input::InputManager {
 public:
  explicit AIDriver(const AIDriverParams& params = {});
  ~AIDriver() override = default;

  // Factory: returns a parameter set tuned for the given difficulty.
  static AIDriverParams difficulty_preset(AIDifficulty diff);

  // Set the track reference used for path planning.
  void set_track(const track::Track& track);

  // Provide the precomputed optimal racing line samples.
  void set_racing_line(const std::vector<track::RacingLineSample>& samples);

  // Main entry point: compute inputs for this simulation step.
  void update(const vehicle::VehicleState& state, double delta_time);

  // Re-apply a difficulty preset (resets all params).
  void set_difficulty(AIDifficulty difficulty);

  // Scale the target corner-entry speed (used for dynamic difficulty).
  void set_target_speed_factor(double factor);

  // Provide the list of all other cars on track for traffic-aware decisions.
  void set_nearby_cars(const std::vector<vehicle::VehicleState>& cars);

  // InputManager overrides.
  input::InputState poll() override;
  bool is_key_down(int key_code) override;

  // Accessors.
  const input::InputState& last_input() const { return last_input_; }
  const AIDriverParams& params() const { return params_; }

 private:
  // --- Configuration -------------------------------------------------------
  AIDriverParams params_;

  // --- Output (cached, returned by poll()) --------------------------------
  input::InputState last_input_;

  // --- Track & racing line ------------------------------------------------
  const track::Track* track_ = nullptr;
  std::vector<track::RacingLineSample> racing_line_;
  bool use_racing_line_ = false;

  // --- Path planning state ------------------------------------------------
  Vec2 current_target_{0.0, 0.0};
  double current_target_speed_ = 0.0;
  double steering_integral_ = 0.0;
  double prev_speed_error_ = 0.0;
  bool has_target_ = false;

  // --- Reaction delay -----------------------------------------------------
  double delay_timer_ = 0.0;

  // --- Human error (RNG) --------------------------------------------------
  std::mt19937 rng_;

  // --- Traffic adaptation state ------------------------------------------
  std::vector<vehicle::VehicleState> nearby_cars_;
  double stuck_behind_timer_ = 0.0;   // seconds stuck behind a slower car
  double overtake_urgency_ = 0.0;     // [0,1] accumulated desire to overtake

  // --- Core computation phases --------------------------------------------
  // Compute the target position and speed, including racing-line lookup
  // and overtaking/defense adjustments.
  void compute_target(const vehicle::VehicleState& state);

  // Convert the target position into a normalized steering command [-1,1].
  void compute_steering(const vehicle::VehicleState& state);

  // Compute throttle and brake from the speed error, with corner-speed
  // anticipation.
  void compute_throttle_brake(const vehicle::VehicleState& state, double delta_time);

  // Select upshift / downshift based on current RPM.
  void compute_gears(const vehicle::VehicleState& state);

  // --- Track geometry helpers --------------------------------------------
  // Sample the track at the vehicle's position and return a target speed
  // based on curvature and surface friction.
  double get_track_target_speed(const Vec2& position) const;

  // Return the curve radius at a given track distance (1/curvature).
  double curve_radius(const Vec2& pos, double distance) const;

  // Find the nearest racing-line sample to the given position.
  const track::RacingLineSample* lookup_racing_line(const Vec2& position) const;

  // --- Traffic & tactical driving ---------------------------------------
  // Inspect nearby cars and update internal traffic state (stuck timer,
  // overtake urgency).  Must run before compute_target so its results
  // can influence the target selection.
  void detect_traffic(const vehicle::VehicleState& state, double delta_time);

  // Return a speed reduction (m/s) for collision avoidance when a slower
  // car is directly ahead within the safety envelope.
  double traffic_speed_adjustment(const vehicle::VehicleState& state) const;

  // Shift the target laterally to plan an overtaking maneuver on a straight.
  // Amplified by overtake_urgency_ when the AI has been stuck behind.
  Vec2 adjust_for_overtaking(const Vec2& target, const vehicle::VehicleState& state, double curvature);

  // Shift the target laterally to defend position against an attacker
  // approaching from alongside in a corner.
  Vec2 adjust_for_defense(const Vec2& target, const vehicle::VehicleState& state, double curvature);

  // --- Human error simulation -------------------------------------------
  // Blend the freshly-computed input with the previous output to simulate
  // a human-like reaction delay.
  void apply_reaction_delay(input::InputState& out, const input::InputState& previous, double delta_time);

  // Add random steering jitter and throttle variance for realism.
  void apply_human_errors(input::InputState& out);
};

}
