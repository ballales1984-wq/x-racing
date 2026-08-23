#include "simulation/simulation.h"
#include "physics/types.h"
#include "weather/weather.h"
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
  state_.front_tire_temp = vehicle_params_.ambient_temperature;
  state_.rear_tire_temp = vehicle_params_.ambient_temperature;
  state_.front_tire_wear = 1.0;
  state_.rear_tire_wear = 1.0;
  state_.fl_tire_load = 0.0;
  state_.fr_tire_load = 0.0;
  state_.rl_tire_load = 0.0;
  state_.rr_tire_load = 0.0;
  state_.body_roll = 0.0;
  state_.body_pitch = 0.0;
  state_.weather_grip_factor = 1.0;
  state_.track_temp = 305.0;
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
    update_weather();
    update_tire_temperature();
    update_suspension();
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
// Includes auto-shift and soft RPM limiter.
void Simulation::update_engine_forces() {
  const double speed = state_.velocity.norm();

  if (speed < kEpsilon) {
    state_.rpm = vehicle_params_.idle_rpm;
    const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
    const double torque = vehicle_params_.max_torque * state_.throttle * 0.8;
    const double engine_force = torque / vehicle_params_.wheel_radius;
    state_.acceleration = forward_dir * engine_force / vehicle_params_.mass;
    return;
  }

  auto compute_rpm = [&](int gear) {
    const double gear_ratio = vehicle_params_.gear_ratios[std::clamp(gear - 1, 0,
      static_cast<int>(vehicle_params_.gear_ratios.size()) - 1)];
    const double total_ratio = gear_ratio * vehicle_params_.final_drive;
    const double wheel_rpm = (speed / (2.0 * kPi * vehicle_params_.wheel_radius)) * 60.0;
    return std::clamp(wheel_rpm * total_ratio, vehicle_params_.idle_rpm, vehicle_params_.max_rpm);
  };

  state_.rpm = compute_rpm(state_.gear);

  if (state_.rpm >= vehicle_params_.max_rpm * 0.92 && state_.gear < static_cast<int>(vehicle_params_.gear_ratios.size())) {
    state_.gear++;
    state_.rpm = compute_rpm(state_.gear);
  } else if (state_.rpm <= vehicle_params_.idle_rpm * 1.8 && state_.gear > 1) {
    state_.gear--;
    state_.rpm = compute_rpm(state_.gear);
  }

  double torque = 0.0;
  const double rpm_frac = (state_.rpm - vehicle_params_.idle_rpm) /
                          (vehicle_params_.max_rpm - vehicle_params_.idle_rpm);

  if (state_.throttle > 0.0) {
    const double curve_input = std::max(rpm_frac, 0.05);
    torque = vehicle_params_.max_torque * state_.throttle * std::sin(curve_input * kHalfPi);
  }

  if (state_.rpm >= vehicle_params_.max_rpm) {
    torque *= vehicle_params_.max_rpm / state_.rpm;
  }

  if (state_.throttle <= 0.0) {
    const double engine_brake_torque = -0.04 * state_.rpm;
    torque += engine_brake_torque;
  }

  const double gear_ratio = vehicle_params_.gear_ratios[std::clamp(state_.gear - 1, 0,
    static_cast<int>(vehicle_params_.gear_ratios.size()) - 1)];
  const double total_ratio = gear_ratio * vehicle_params_.final_drive;
  double engine_force = (torque * total_ratio) / vehicle_params_.wheel_radius;
  const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  state_.acceleration = forward_dir * engine_force / vehicle_params_.mass;
}

// Aerodynamic model with dynamic ride height, pitch and roll sensitivity.
//
// Computes:
//   Drag: D = 0.5 * rho * Cd * A * v^2
//   Lift: L = 0.5 * rho * Cl * A * v^2
//   Downforce: DF = 0.5 * rho * Cz * A * v^2 * ride_height_factor
//
// Downforce depends on:
//   - ride height (lower = more downforce)
//   - body pitch (affects front/rear balance)
//   - body roll (affects total downforce)
//   - wing angle (adjustable setup parameter)
void Simulation::update_aerodynamics() {
  const double speed = state_.velocity.norm();
  if (speed < kEpsilon) {
    state_.aero_drag = 0.0;
    state_.aero_lift = 0.0;
    state_.aero_downforce = 0.0;
    return;
  }

  const double rho = 1.225;
  const double dynamic_pressure = 0.5 * rho * speed * speed;

  const double base_drag = dynamic_pressure * vehicle_params_.frontal_area * vehicle_params_.drag_coefficient;
  const double base_lift = dynamic_pressure * vehicle_params_.frontal_area * vehicle_params_.lift_coefficient;
  const double base_downforce = dynamic_pressure * vehicle_params_.frontal_area * vehicle_params_.downforce_coefficient;

  const double ride_height_factor = 1.0 + vehicle_params_.ride_height_sensitivity * state_.body_pitch;
  const double pitch_factor = 1.0 - vehicle_params_.pitch_sensitivity * std::abs(state_.body_pitch);
  const double roll_factor = 1.0 - vehicle_params_.roll_sensitivity * std::abs(state_.body_roll);
  const double wing_factor = 1.0 + std::sin(vehicle_params_.wing_angle);

  const double drag = base_drag * (1.0 + vehicle_params_.wing_angle * 0.1);
  const double lift = base_lift * ride_height_factor * roll_factor;
  const double downforce = base_downforce * ride_height_factor * pitch_factor * roll_factor * wing_factor;

  const Vec2 drag_dir = -state_.velocity.normalized();
  const double drag_decel = drag / vehicle_params_.mass;

  state_.acceleration += drag_dir * drag_decel;
  state_.aero_drag = drag;
  state_.aero_lift = lift;
  state_.aero_downforce = downforce;
  state_.aero_front_balance = 0.5 - vehicle_params_.pitch_sensitivity * state_.body_pitch;
}

// Weather effects on vehicle and track.
//
// Simplified weather model that affects:
//   - Tire grip (rain reduces friction)
//   - Tire temperature (rain cools, wind affects heat transfer)
//   - Track friction (wet surface reduces grip)
//   - Rolling resistance (wet surface increases resistance)
//   - Wind effect on vehicle speed
//
// The model is deterministic and suitable for real-time simulation.
// It captures the essential effects of weather on driving dynamics.
void Simulation::update_weather() {
  const double rain = vehicle_params_.rain_intensity;

  // Rain cools tires and track
  if (rain > 0.0) {
    const double rain_cooling = vehicle_params_.rain_cooling * rain;
    state_.front_tire_temp -= rain_cooling * (params_.dt / params_.substeps);
    state_.rear_tire_temp -= rain_cooling * (params_.dt / params_.substeps);
    state_.track_temp -= rain_cooling * 0.5 * (params_.dt / params_.substeps);
  }

  // Track temperature approaches air temperature
  const double track_cooling = vehicle_params_.temp_cooling_rate * (state_.track_temp - vehicle_params_.ambient_temperature);
  state_.track_temp -= track_cooling * (params_.dt / params_.substeps);

  // Track heating from sun (simplified)
  state_.track_temp += vehicle_params_.track_heat_rate * (params_.dt / params_.substeps);

  // Update weather grip factor based on rain
  state_.weather_grip_factor = 1.0 - rain * vehicle_params_.rain_grip_reduction;

  // Wind effect on speed (simplified)
  const double wind_speed = 0.0; // placeholder for future wind implementation
  if (wind_speed > 0.0) {
    const double wind_effect = vehicle_params_.wind_effect_on_speed * wind_speed;
    state_.speed += wind_effect * (params_.dt / params_.substeps);
    state_.speed = clamp(state_.speed, 0.0, 150.0);
  }
}

// Simple tire thermal model.
// Temperature increases with slip and speed, cools towards ambient.
// Wear increases with slip and accumulates per lap.
void Simulation::update_tire_temperature() {
  const double speed = state_.speed;
  const double abs_front_slip = std::abs(state_.front_slip_angle);
  const double abs_rear_slip = std::abs(state_.rear_slip_angle);

  double heat_front = vehicle_params_.tire_heat_per_slip * abs_front_slip + speed * 0.01;
  double heat_rear = vehicle_params_.tire_heat_per_slip * abs_rear_slip + speed * 0.01;

  double cool_front = vehicle_params_.tire_cooling_rate * (state_.front_tire_temp - vehicle_params_.ambient_temperature);
  double cool_rear = vehicle_params_.tire_cooling_rate * (state_.rear_tire_temp - vehicle_params_.ambient_temperature);

  state_.front_tire_temp += (heat_front - cool_front) * (params_.dt / params_.substeps);
  state_.rear_tire_temp += (heat_rear - cool_rear) * (params_.dt / params_.substeps);

  double wear_rate = vehicle_params_.tire_wear_per_slip * (abs_front_slip + abs_rear_slip) * 0.5;
  state_.front_tire_wear -= wear_rate * (params_.dt / params_.substeps);
  state_.rear_tire_wear -= wear_rate * (params_.dt / params_.substeps);

  state_.front_tire_wear = clamp(state_.front_tire_wear, 0.0, 1.0);
  state_.rear_tire_wear = clamp(state_.rear_tire_wear, 0.0, 1.0);
}

// Simplified suspension model: quasi-static weight transfer.
//
// Calculates 4-corner normal forces based on:
//   - Static load distribution (from CG position)
//   - Longitudinal weight transfer (acceleration/braking)
//   - Lateral weight transfer (cornering)
//   - Anti-roll bar effect
//
// The model is fast and deterministic, suitable for real-time simulation.
// It captures the essential effect of suspension on tire grip without
// requiring full multibody dynamics.
void Simulation::update_suspension() {
  const double m = vehicle_params_.mass;
  const double g = kGravity;
  const double a_f = vehicle_params_.cg_to_front;
  const double a_r = vehicle_params_.cg_to_rear;
  const double L = vehicle_params_.wheelbase;
  const double T = vehicle_params_.track_width;
  const double h = vehicle_params_.cg_height;

  const double a_long = state_.acceleration.dot(Vec2(std::cos(state_.heading), std::sin(state_.heading)));
  const double a_lat = state_.acceleration.dot(Vec2(-std::sin(state_.heading), std::cos(state_.heading)));

  double static_front = m * g * (a_r / L);
  double static_rear = m * g * (a_f / L);

  const double total_aero_load = state_.aero_downforce - state_.aero_lift;
  double aero_front = total_aero_load * state_.aero_front_balance;
  double aero_rear = total_aero_load * (1.0 - state_.aero_front_balance);

  double dFz_long = (m * a_long * h) / L;
  double dFz_lat = (m * a_lat * h) / T;

  double fl_load = (static_front * 0.5 + aero_front * 0.5) + dFz_long * 0.5 + dFz_lat * 0.5;
  double fr_load = (static_front * 0.5 + aero_front * 0.5) + dFz_long * 0.5 - dFz_lat * 0.5;
  double rl_load = (static_rear * 0.5 + aero_rear * 0.5) - dFz_long * 0.5 + dFz_lat * 0.5;
  double rr_load = (static_rear * 0.5 + aero_rear * 0.5) - dFz_long * 0.5 - dFz_lat * 0.5;

  fl_load = std::max(fl_load, m * g * 0.05);
  fr_load = std::max(fr_load, m * g * 0.05);
  rl_load = std::max(rl_load, m * g * 0.05);
  rr_load = std::max(rr_load, m * g * 0.05);

  const double anti_roll_moment = vehicle_params_.anti_roll_bar_stiffness * a_lat;
  const double anti_roll_distribution = anti_roll_moment / T;

  fl_load -= anti_roll_distribution * 0.5;
  fr_load += anti_roll_distribution * 0.5;
  rl_load -= anti_roll_distribution * 0.5;
  rr_load += anti_roll_distribution * 0.5;

  state_.fl_tire_load = std::max(fl_load, 0.0);
  state_.fr_tire_load = std::max(fr_load, 0.0);
  state_.rl_tire_load = std::max(rl_load, 0.0);
  state_.rr_tire_load = std::max(rr_load, 0.0);

  state_.body_roll = a_lat * h / (vehicle_params_.front_spring_rate + vehicle_params_.rear_spring_rate);
  state_.body_pitch = -a_long * h / (vehicle_params_.front_spring_rate + vehicle_params_.rear_spring_rate);
}

// Bicycle model tire forces with Pacejka lateral grip.
// Tire loads come from the suspension model (update_suspension).
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
  state_.slip_angle = (std::abs(alpha_f) + std::abs(alpha_r)) * 0.5;

  const double m = vehicle_params_.mass;
  const double g = kGravity;

  auto tire_grip_factor = [](double temp, double wear, const vehicle::VehicleParams& p) {
    double temp_diff = temp - p.tire_optimal_temp;
    double temp_factor = std::exp(-0.5 * std::pow(temp_diff / p.tire_temp_curve_width, 2));
    double wear_factor = 0.5 + 0.5 * wear;
    return temp_factor * wear_factor;
  };

  double grip_front = tire_grip_factor(state_.front_tire_temp, state_.front_tire_wear, vehicle_params_);
  double grip_rear = tire_grip_factor(state_.rear_tire_temp, state_.rear_tire_wear, vehicle_params_);
  double mu_front = vehicle_params_.tire_mu * grip_front;
  double mu_rear = vehicle_params_.tire_mu * grip_rear;

  const auto& tp = track_->at(state_.distance_along_track);
  const double banking_factor = std::cos(tp.banking);
  const double friction_factor = tp.friction * state_.weather_grip_factor;

  double fz_front = state_.fl_tire_load + state_.fr_tire_load;
  double fz_rear = state_.rl_tire_load + state_.rr_tire_load;

  const double f_yf_raw = mu_front * fz_front * friction_factor * banking_factor *
    physics::pacejka_tire_model(alpha_f, 1.0,
      vehicle_params_.tire_pacejka_b, vehicle_params_.tire_pacejka_c,
      vehicle_params_.tire_pacejka_e);

  const double f_yr_raw = mu_rear * fz_rear * friction_factor * banking_factor *
    physics::pacejka_tire_model(alpha_r, 1.0,
      vehicle_params_.tire_pacejka_b, vehicle_params_.tire_pacejka_c,
      vehicle_params_.tire_pacejka_e);

  const double f_y_total = f_yf_raw + f_yr_raw;
  const double f_y_max = (mu_front + mu_rear) * 0.5 * m * g * banking_factor * friction_factor;

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
  const double damping_coeff = vehicle_params_.front_damping / vehicle_params_.mass;
  new_lat_speed -= damping_coeff * state_.lateral_velocity * dt;
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
