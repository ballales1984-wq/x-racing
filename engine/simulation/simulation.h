#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include "track/track.h"
#include "track/lap_detector.h"
#include "input/input.h"
#include "physics/types.h"
#include "physics/tire_model.h"

// Project 0 — simulation loop
// Namespace: p0::simulation
namespace p0::simulation {

// Simulation configuration
struct SimulationParams {
  double dt = 1.0 / 60.0;                // s, base timestep (60 Hz)
  double substeps = 4.0;                 // number of physics sub-steps per frame
  bool use_abs = true;                   // enable ABS (placeholder)
  bool use_tcs = true;                   // enable TCS (placeholder)
};

// Result of a single simulation step
struct SimulationResult {
  vehicle::VehicleState state;           // vehicle state after step
  bool collision = false;                // collision detected (stuck off-track >2s)
  bool off_track = false;                // vehicle outside track bounds
  double time = 0.0;                     // accumulated simulation time
};

// Core simulation loop: physics + integration + track constraint checks.
// The simulation is independent of any renderer; it can run headless.
class Simulation {
 public:
  explicit Simulation(const SimulationParams& params = {});
  ~Simulation() = default;

  void set_track(const track::Track& track);
  void reset(const vehicle::VehicleState& initial_state);
  SimulationResult step(const input::InputState& input);
  // Reset the car to the last valid on-track position.
  void respawn();

  const vehicle::VehicleState& state() const { return state_; }
  const track::Track& track() const {
    assert(track_ && "Simulation::track() called before set_track()");
    return *track_;
  }
  vehicle::VehicleParams& mutable_params() { return vehicle_params_; }

  // Maximum reverse speed (m/s) — keeps reverse as a low-speed maneuver.
  double max_reverse_speed() const { return max_reverse_speed_; }
  void set_max_reverse_speed(double v) { max_reverse_speed_ = std::max(0.0, v); }
  bool is_reversing() const { return reversing_; }

 private:
  // Physics update stages (called in order each sub-step)
  void update_engine_forces(const input::InputState& input);
  void update_aerodynamics();             // drag and lift
  void update_weather();                  // rain, wind, temperature effects
  void update_tire_temperature();         // tire thermal model + wear
  void update_suspension();               // spring-damper + weight transfer
  void update_tire_forces(double dt);     // Pacejka forces with dynamic Fz
  void update_centripetal_forces();       // centripetal/centrifugal forces on curves
  void integrate(double dt);              // velocity-space integration
  void apply_box_lane_speed_limit();      // enforce box lane speed limit
  void apply_off_track_physics();         // grip reduction + barrier push when off track
  void update_fuel_consumption();         // fuel consumption model

  SimulationParams params_;
  const track::Track* track_ = nullptr;
  vehicle::VehicleState state_;
  vehicle::VehicleParams vehicle_params_;
  double total_time_ = 0.0;

  // Last state recorded while the car was on-track (used for respawn).
  vehicle::VehicleState last_valid_state_;
  bool has_valid_state_ = false;
  double frames_off_track_ = 0;  // consecutive off-track frames

  // Reverse handling: engaged only when nearly stopped, cleared at speed.
  bool reversing_ = false;
  double max_reverse_speed_ = 8.0;  // m/s, low-speed reverse cap

  // Runtime lap detection with forward-direction validation.
  track::LapDetector lap_detector_{0.0};
  double prev_slip_ratio_ = 0.0;
};

}
