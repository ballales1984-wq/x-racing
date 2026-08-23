#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include "track/track.h"
#include "input/input.h"

namespace p0::simulation {

struct SimulationParams {
  double dt = 1.0 / 120.0;
  double substeps = 4.0;
  bool use_abs = true;
  bool use_tcs = true;
};

struct SimulationResult {
  vehicle::VehicleState state;
  bool collision = false;
  bool off_track = false;
  double time = 0.0;
};

class Simulation {
 public:
  explicit Simulation(const SimulationParams& params = {});
  ~Simulation() = default;

  void set_track(const track::Track& track);
  void reset(const vehicle::VehicleState& initial_state);
  SimulationResult step(const input::InputState& input);

  const vehicle::VehicleState& state() const { return state_; }
  const track::Track& track() const { return *track_; }

 private:
  void update_engine_forces();
  void update_aerodynamics();
  void update_tire_forces();
  void update_braking();
  void update_steering();
  void integrate(double dt);

  SimulationParams params_;
  const track::Track* track_ = nullptr;
  vehicle::VehicleState state_;
  vehicle::VehicleParams vehicle_params_;
};

}
