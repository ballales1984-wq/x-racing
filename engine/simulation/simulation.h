#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include "track/track.h"
#include "input/input.h"

// Project 0 — simulation loop
// Namespace: p0::simulation
namespace p0::simulation {

// Simulation configuration
struct SimulationParams {
  double dt = 1.0 / 120.0;               // s, base timestep (120 Hz)
  double substeps = 4.0;                 // number of physics sub-steps per frame
  bool use_abs = true;                   // enable ABS (placeholder)
  bool use_tcs = true;                   // enable TCS (placeholder)
};

// Result of a single simulation step
struct SimulationResult {
  vehicle::VehicleState state;           // vehicle state after step
  bool collision = false;                // collision detected (placeholder)
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

  const vehicle::VehicleState& state() const { return state_; }
  const track::Track& track() const { return *track_; }
  vehicle::VehicleParams& mutable_params() { return vehicle_params_; }

 private:
  // Physics update stages (called in order each sub-step)
  void update_engine_forces();            // engine torque -> longitudinal force
  void update_aerodynamics();             // drag and lift
  void update_weather();                  // rain, wind, temperature effects
  void update_tire_temperature();         // tire thermal model + wear
  void update_suspension();               // spring-damper + weight transfer
  void update_tire_forces(double dt);              // Pacejka forces with dynamic Fz
  void update_braking();                  // brake deceleration
  void update_steering();                 // bicycle model slip angles + lateral forces
  void update_centripetal_forces();       // centripetal/centrifugal forces on curves
  void integrate(double dt);              // velocity-space integration
  void apply_box_lane_speed_limit();      // enforce box lane speed limit

  SimulationParams params_;
  const track::Track* track_ = nullptr;
  vehicle::VehicleState state_;
  vehicle::VehicleParams vehicle_params_;
  double total_time_ = 0.0;
};

}
