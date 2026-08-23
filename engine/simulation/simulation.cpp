#include "simulation/simulation.h"
#include "physics/types.h"
#include <algorithm>
#include <cmath>

// Project 0 — simulation loop implementation
// The simulation runs at 120 Hz with 4 sub-steps per frame.
// Each sub-step applies: engine -> aero -> tires -> brakes -> steering -> integrate.
// M3 introduces lateral tire forces via the bicycle model with Pacejka grip.
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
  state_.lateral_velocity = 0.0;
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
    state_.steer_angle = input.steering * vehicle_params_.max_steer_angle;
    state_.throttle = clamp(input.throttle, 0.0, 1.0);
    state_.brake = clamp(input.brake, 0.0, 1.0);

    if (input.upshift && state_.gear < static_cast<int>(vehicle_params_.gear_ratios.size())) {
      state_.gear++;
    }
    if (input.downshift && state_.gear > 1) {
      state_.gear--;
    }

    update_engine_forces();
    update_aerodynamics();
    update_tire_forces();
    update_braking();
    update_steering();
    integrate(dt);
  }

  const auto& tp = track_->at(state_.distance_along_track);
  const Vec2 to_car = state_.position - tp.position;
  const double lateral = to_car.dot(tp.normal);
  const double track_half = tp.width / 2.0;

  total_time_ += params_.dt;
  SimulationResult result;
  result.state = state_;
  result.time = total_time_;
  result.off_track = std::abs(lateral) > track_half;
  result.collision = false;

  return result;
}

// Engine model: torque curve based on RPM, mapped to longitudinal force.
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
void Simulation::update_aerodynamics() {
  const double speed = state_.velocity.norm();
  if (speed < kEpsilon) return;

  const double rho = 1.225;
  const double drag_force = 0.5 * rho * vehicle_params_.frontal_area *
                            vehicle_params_.drag_coefficient * speed * speed;

  const Vec2 drag_dir = -state_.velocity.normalized();
  const double drag_decel = drag_force / vehicle_params_.mass;

  state_.acceleration += drag_dir * drag_decel;
}

// Bicycle model tire forces with Pacejka lateral grip.
//
// The vehicle is modeled as a single track (bicycle) with:
//   - front axle at distance a_f from CG
//   - rear axle at distance a_r from CG
//
// Slip angles:
//   alpha_f = delta - atan((v_y + a_f * omega) / v_x)
//   alpha_r = -atan((v_y - a_r * omega) / v_x)
//
// Lateral forces:
//   F_yf = Pacejka(alpha_f)
//   F_yr = Pacejka(alpha_r)
//
// Yaw moment:
//   M_z = a_f * F_yf - a_r * F_yr
//
// If total lateral force exceeds available grip (mu * m * g),
// forces are proportionally limited (understeer behavior).
void Simulation::update_tire_forces() {
  const double speed = state_.speed;
  if (speed < kEpsilon) return;

  const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  const Vec2 right_dir(-forward_dir.y(), forward_dir.x());

  const double v_x = speed;
  const double v_y = state_.lateral_velocity;
  const double omega = state_.yaw_rate;

  const double a_f = vehicle_params_.cg_to_front;
  const double a_r = vehicle_params_.cg_to_rear;
  const double delta = state_.steer_angle;

  double alpha_f = delta - std::atan2(v_y + a_f * omega, v_x);
  double alpha_r = -std::atan2(v_y - a_r * omega, v_x);

  state_.front_slip_angle = alpha_f;
  state_.rear_slip_angle = alpha_r;

  const double mu = vehicle_params_.tire_mu;
  const double m = vehicle_params_.mass;
  const double g = kGravity;

  const double fz_front = m * g * (a_r / (a_f + a_r));
  const double fz_rear = m * g * (a_f / (a_f + a_r));

  const double f_yf_raw = mu * fz_front *
    physics::pacejka_tire_model(alpha_f, 1.0,
      vehicle_params_.tire_pacejka_b, vehicle_params_.tire_pacejka_c,
      vehicle_params_.tire_pacejka_e);

  const double f_yr_raw = mu * fz_rear *
    physics::pacejka_tire_model(alpha_r, 1.0,
      vehicle_params_.tire_pacejka_b, vehicle_params_.tire_pacejka_c,
      vehicle_params_.tire_pacejka_e);

  const double f_y_total = f_yf_raw + f_yr_raw;
  const double f_y_max = mu * m * g;

  double f_yf = f_yf_raw;
  double f_yr = f_yr_raw;

  if (std::abs(f_y_total) > f_y_max && std::abs(f_y_total) > kEpsilon) {
    const double scale = f_y_max / std::abs(f_y_total);
    f_yf *= scale;
    f_yr *= scale;
  }

  const double m_z = a_f * f_yf - a_r * f_yr;

  const Vec2 lateral_force = right_dir * (f_yf + f_yr);
  state_.acceleration += lateral_force / m;

  state_.yaw_rate = m_z / (m * (a_f * a_f + a_r * a_r) * 0.5);
}

// Simple braking model: constant deceleration proportional to brake pedal.
void Simulation::update_braking() {
  const double speed = state_.velocity.norm();
  if (speed < kEpsilon) return;

  const double brake_decel = (vehicle_params_.max_brake_force * state_.brake) / vehicle_params_.mass;
  const Vec2 brake_dir = -state_.velocity.normalized();
  state_.acceleration += brake_dir * brake_decel;
}

// Steering input maps to front wheel angle.
// The actual turning behavior emerges from the bicycle model in update_tire_forces().
void Simulation::update_steering() {
  const double speed = state_.speed;
  if (speed < kEpsilon) {
    state_.yaw_rate = 0.0;
    return;
  }
}

// Velocity-space integration.
// The velocity vector is decomposed into longitudinal and lateral components.
// Longitudinal component drives speed; lateral component drives slip dynamics.
// This allows the bicycle model to produce realistic understeer/oversteer.
void Simulation::integrate(double dt) {
  const double speed = state_.speed;
  const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  const Vec2 right_dir(-forward_dir.y(), forward_dir.x());

  const double a_long = state_.acceleration.dot(forward_dir);
  const double a_lat = state_.acceleration.dot(right_dir);

  double new_long_speed = speed + a_long * dt;
  new_long_speed = clamp(new_long_speed, 0.0, 150.0);

  double new_lat_speed = state_.lateral_velocity + a_lat * dt;
  const double lat_speed_max = 30.0;
  new_lat_speed = clamp(new_lat_speed, -lat_speed_max, lat_speed_max);

  state_.heading += state_.yaw_rate * dt;
  state_.heading = normalize_angle(state_.heading);

  const Vec2 new_forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  state_.velocity = new_forward_dir * new_long_speed + right_dir * new_lat_speed;
  state_.position += state_.velocity * dt;
  state_.speed = new_long_speed;
  state_.lateral_velocity = new_lat_speed;
  state_.acceleration = Vec2::Zero();

  if (track_) {
    state_.distance_along_track += new_long_speed * dt;
    if (state_.distance_along_track >= track_->length()) {
      state_.distance_along_track -= track_->length();
      state_.lap += 1;
    }
  }
}

}
