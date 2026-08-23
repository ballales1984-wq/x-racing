#include "simulation/simulation.h"
#include "physics/types.h"
#include <algorithm>
#include <cmath>

// Project 0 — simulation loop implementation
// The simulation runs at 120 Hz with 4 sub-steps per frame.
// Each sub-step applies: engine -> aero -> tires -> brakes -> steering -> integrate.
// The bicycle model governs lateral dynamics; tire forces provide longitudinal grip.
namespace p0::simulation {

Simulation::Simulation(const SimulationParams& params) : params_(params) {}

void Simulation::set_track(const track::Track& track) {
  track_ = &track;
}

void Simulation::reset(const vehicle::VehicleState& initial_state) {
  state_ = initial_state;
  state_.distance_along_track = 0.0;
  state_.lap = 0;
  state_.rpm = vehicle_params_.idle_rpm;
  state_.gear = 1;
}

// Advance the simulation by one frame (params_.dt seconds).
// Internally subdivided into `substeps` smaller steps for stability.
SimulationResult Simulation::step(const input::InputState& input) {
  if (!track_) {
    SimulationResult result;
    result.state = state_;
    return result;
  }

  const double dt = params_.dt / params_.substeps;

  for (int sub = 0; sub < static_cast<int>(params_.substeps); ++sub) {
    // Map normalized input to physical values
    state_.steer_angle = input.steering * vehicle_params_.max_steer_angle;
    state_.throttle = clamp(input.throttle, 0.0, 1.0);
    state_.brake = clamp(input.brake, 0.0, 1.0);

    update_engine_forces();
    update_aerodynamics();
    update_tire_forces();
    update_braking();
    update_steering();
    integrate(dt);
  }

  // Track constraint: off-track if vehicle center is outside track width
  const auto& tp = track_->at(state_.distance_along_track);
  const Vec2 to_car = state_.position - tp.position;
  const double lateral = to_car.dot(tp.normal);
  const double track_half = tp.width / 2.0;

  SimulationResult result;
  result.state = state_;
  result.time += params_.dt;
  result.off_track = std::abs(lateral) > track_half;
  result.collision = false;

  return result;
}

// Engine model: torque curve based on RPM, mapped to longitudinal force.
// Simplified: no turbo lag, no inertia, no engine braking.
void Simulation::update_engine_forces() {
  const double speed = state_.velocity.norm();
  state_.rpm = vehicle_params_.idle_rpm;

  if (speed < kEpsilon) {
    const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
    const double torque = vehicle_params_.max_torque * state_.throttle * 0.8;
    const double engine_force = torque / vehicle_params_.wheel_radius;
    state_.acceleration = forward_dir * engine_force / vehicle_params_.mass;
    return;
  }

  double gear_ratio = vehicle_params_.gear_ratios[std::clamp(state_.gear - 1, 0,
    static_cast<int>(vehicle_params_.gear_ratios.size()) - 1)];
  double total_ratio = gear_ratio * vehicle_params_.final_drive;
  double wheel_rpm = (speed / (2.0 * kPi * vehicle_params_.wheel_radius)) * 60.0;
  state_.rpm = std::clamp(wheel_rpm * total_ratio, vehicle_params_.idle_rpm, vehicle_params_.max_rpm);

  double torque = 0.0;
  const double rpm_frac = (state_.rpm - vehicle_params_.idle_rpm) /
                          (vehicle_params_.max_rpm - vehicle_params_.idle_rpm);

  if (state_.throttle > 0.0) {
    torque = vehicle_params_.max_torque * state_.throttle * std::sin(rpm_frac * kHalfPi);
  }

  double engine_force = (torque * total_ratio) / vehicle_params_.wheel_radius;
  const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  state_.acceleration = forward_dir * engine_force / vehicle_params_.mass;
}

// Aerodynamic drag and lift (quadratic in speed).
// Lift is computed but not yet coupled to suspension/weight transfer.
void Simulation::update_aerodynamics() {
  const double speed = state_.velocity.norm();
  if (speed < kEpsilon) return;

  const double rho = 1.225;
  const double drag_force = 0.5 * rho * vehicle_params_.frontal_area *
                            vehicle_params_.drag_coefficient * speed * speed;
  const double lift_force = 0.5 * rho * vehicle_params_.frontal_area *
                            vehicle_params_.lift_coefficient * speed * speed;

  const Vec2 drag_dir = -state_.velocity.normalized();
  const double drag_decel = drag_force / vehicle_params_.mass;

  state_.acceleration += drag_dir * drag_decel;
}

// Longitudinal tire forces using Pacejka model.
// Lateral grip is handled by the bicycle model in update_steering().
// Slip ratio is recorded for telemetry; lateral slip angle is computed but
// the lateral force itself is not applied here (M3 placeholder).
void Simulation::update_tire_forces() {
  const double speed = state_.velocity.norm();
  if (speed < kEpsilon) return;

  const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  const Vec2 right_dir(-forward_dir.y(), forward_dir.x());

  const double long_vel = state_.velocity.dot(forward_dir);
  const double lat_vel = state_.velocity.dot(right_dir);

  double long_slip = 0.0;
  double lat_slip = 0.0;

  if (std::abs(long_vel) > kEpsilon) {
    long_slip = (long_vel - vehicle_params_.wheel_radius * state_.rpm * kTwoPi / 60.0 * 0.1) / std::abs(long_vel);
    long_slip = clamp(long_slip, -1.0, 1.0);
  }

  if (speed > kEpsilon) {
    lat_slip = std::atan2(lat_vel, std::abs(long_vel));
  }

  state_.slip_ratio = long_slip;
  state_.slip_angle = lat_slip;

  const double mu = vehicle_params_.tire_mu;
  const double max_grip = mu * vehicle_params_.mass * kGravity;
  const double long_force = physics::pacejka_tire_model(long_slip, max_grip,
    vehicle_params_.tire_pacejka_b, vehicle_params_.tire_pacejka_c,
    vehicle_params_.tire_pacejka_e);

  const Vec2 traction_force = forward_dir * long_force;
  state_.acceleration += traction_force / vehicle_params_.mass;
}

// Simple braking model: constant deceleration proportional to brake pedal.
// ABS/TCS are placeholders for future implementation.
void Simulation::update_braking() {
  const double speed = state_.velocity.norm();
  if (speed < kEpsilon) return;

  const double brake_decel = (vehicle_params_.max_brake_force * state_.brake) / vehicle_params_.mass;
  const Vec2 brake_dir = -state_.velocity.normalized();
  state_.acceleration += brake_dir * brake_decel;
}

// Bicycle model steering with kinematic yaw rate and grip-limited understeer.
//
//   omega = v * tan(delta) / L
//
// If the resulting lateral acceleration exceeds tire grip (mu * g),
// the yaw rate is capped to prevent impossible cornering.
// This produces natural understeer behavior: the car "plows" when
// the demanded lateral force exceeds available friction.
void Simulation::update_steering() {
  const double speed = state_.speed;
  if (speed < kEpsilon) {
    state_.yaw_rate = 0.0;
    return;
  }

  const double delta = state_.steer_angle;
  const double L = vehicle_params_.wheelbase;

  double omega = speed * std::tan(delta) / L;

  const double a_y = speed * omega;
  const double a_y_max = vehicle_params_.tire_mu * kGravity;

  if (std::abs(a_y) > a_y_max) {
    omega = (a_y > 0.0 ? 1.0 : -1.0) * a_y_max / speed;
  }

  state_.yaw_rate = omega;
}

// Semi-implicit Euler integration.
// Speed is updated from longitudinal acceleration, then heading from yaw rate,
// then position from the new heading and speed. This ordering provides
// better energy behavior than explicit Euler.
void Simulation::integrate(double dt) {
  const double speed = state_.speed;
  const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  const double a_long = state_.acceleration.dot(forward_dir);

  double new_speed = speed + a_long * dt;
  new_speed = clamp(new_speed, 0.0, 150.0);

  state_.heading += state_.yaw_rate * dt;
  state_.heading = normalize_angle(state_.heading);

  const Vec2 new_forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  state_.velocity = new_forward_dir * new_speed;
  state_.position += state_.velocity * dt;
  state_.speed = new_speed;
  state_.acceleration = Vec2::Zero();

  if (track_) {
    state_.distance_along_track += new_speed * dt;
    if (state_.distance_along_track >= track_->length()) {
      state_.distance_along_track -= track_->length();
      state_.lap += 1;
    }
  }
}

}
