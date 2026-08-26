#include "simulation/simulation.h"
#include "physics/types.h"
#include "physics/tire_model.h"
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
  state_.front_slip_angle_relaxed = 0.0;
  state_.rear_slip_angle_relaxed = 0.0;
  state_.front_camber = 0.0;
  state_.rear_camber = 0.0;
  state_.centripetal_force = 0.0;
  state_.centrifugal_force = 0.0;
  state_.lateral_g = 0.0;
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
    state_.acceleration = Vec2::Zero();
    state_.steer_angle = input.steering * vehicle_params_.max_steer_angle;
    state_.throttle = clamp(input.throttle, 0.0, 1.0);
    state_.brake = clamp(input.brake, 0.0, 1.0);

  if (input.upshift && state_.gear < static_cast<int>(vehicle_params_.gear_ratios.size())) {
    state_.gear++;
  }
  if (input.downshift && state_.gear > 1) {
    state_.gear--;
  }

  state_.box_lane_entry_requested = input.enter_exit_box;

    update_engine_forces();
    update_aerodynamics();
    update_weather();
    update_tire_temperature();
    update_suspension();
    update_tire_forces(dt);
    update_braking();
    update_steering();
    update_centripetal_forces();
    integrate(dt);
  }

  const auto& tp = track_->at(state_.distance_along_track);
  const Vec2 to_car = state_.position - tp.position;
  const double lateral = to_car.dot(tp.normal);
  const double track_half = tp.width / 2.0;

  state_.in_box_lane = false;
  state_.box_lane_speed = 0.0;

  if (tp.has_box_lane) {
    const double box_lane_half = tp.box_lane_width / 2.0;
    const double box_center = -track_half - box_lane_half;
    const double dist_to_box = std::abs(lateral - box_center);
    const bool inside_box = lateral < -track_half && lateral > -track_half - tp.box_lane_width;

    if (inside_box || (state_.in_box_lane && dist_to_box < box_lane_half + 1.0)) {
      state_.in_box_lane = true;
      state_.box_lane_speed = 22.2; // 80 km/h speed limit in box lane
    }

    if (state_.box_lane_entry_requested && !state_.in_box_lane && dist_to_box < box_lane_half + 2.0) {
      state_.in_box_lane = true;
      state_.box_lane_speed = 22.2;
    } else   if (state_.box_lane_entry_requested && state_.in_box_lane && lateral > -track_half + 1.0) {
      state_.in_box_lane = false;
      state_.box_lane_speed = 0.0;
    }
  }

  apply_off_track_physics();
  apply_box_lane_speed_limit();

  // Save last valid on-track state for respawn
  const bool currently_off = std::abs(lateral) > track_half && !state_.in_box_lane;
  if (!currently_off) {
    last_valid_state_ = state_;
    has_valid_state_ = true;
    frames_off_track_ = 0;
  } else {
    frames_off_track_ += 1;
  }

  total_time_ += params_.dt;
  SimulationResult result;
  result.state = state_;
  result.time = total_time_;
  result.off_track = currently_off;
  // Collision: car stuck against barrier for more than ~2 s (240 frames at 120 Hz)
  result.collision = currently_off && frames_off_track_ > 240 && state_.speed < 2.0;

  return result;
}

// Engine model with realistic torque curve, inertia, and drivetrain loss.
// Includes auto-shift with hysteresis and engine braking.
void Simulation::update_engine_forces() {
  const double speed = state_.velocity.norm();
  const double m = vehicle_params_.mass;

  auto compute_rpm = [&](int gear) {
    const double gear_ratio = vehicle_params_.gear_ratios[std::clamp(gear - 1, 0,
      static_cast<int>(vehicle_params_.gear_ratios.size()) - 1)];
    const double total_ratio = gear_ratio * vehicle_params_.final_drive;
    const double wheel_rpm = (speed / (2.0 * kPi * vehicle_params_.wheel_radius)) * 60.0;
    return std::clamp(wheel_rpm * total_ratio, vehicle_params_.idle_rpm, vehicle_params_.max_rpm);
  };

  if (speed < kEpsilon) {
    state_.rpm = vehicle_params_.idle_rpm;
    const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
    const double torque = vehicle_params_.max_torque * state_.throttle * 0.8;
    const double engine_force = torque / vehicle_params_.wheel_radius;
    state_.acceleration = forward_dir * engine_force / m;
    return;
  }

  const double rpm_frac = (state_.rpm - vehicle_params_.idle_rpm) /
                          (vehicle_params_.max_rpm - vehicle_params_.idle_rpm);
  const double clamped_frac = std::clamp(rpm_frac, 0.0, 1.0);

  const double peak_torque_point = 0.55;
  const double width = 0.25;
  const double t = (clamped_frac - peak_torque_point) / width;
  const double shape = std::exp(-0.5 * t * t);
  const double power_band = 1.0 + 0.25 * std::max(0.0, (clamped_frac - 0.7) / 0.3);

  double engine_torque = 0.0;
  if (state_.throttle > 0.0) {
    engine_torque = vehicle_params_.max_torque * state_.throttle * shape * power_band;
  }

  if (state_.rpm >= vehicle_params_.max_rpm) {
    engine_torque *= vehicle_params_.max_rpm / state_.rpm;
  }

  if (state_.throttle <= 0.0) {
    const double engine_brake_torque = -0.04 * state_.rpm;
    engine_torque += engine_brake_torque;
  }

  const double gear_ratio = vehicle_params_.gear_ratios[std::clamp(state_.gear - 1, 0,
    static_cast<int>(vehicle_params_.gear_ratios.size()) - 1)];
  const double total_ratio = gear_ratio * vehicle_params_.final_drive;
  const double wheel_torque = engine_torque * total_ratio * (1.0 - vehicle_params_.drivetrain_loss);
  const double engine_force = wheel_torque / vehicle_params_.wheel_radius;

  const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  state_.acceleration = forward_dir * engine_force / m;

  double target_rpm = compute_rpm(state_.gear);
  const double max_rpm_accel = (vehicle_params_.max_torque / vehicle_params_.engine_inertia) *
                                total_ratio * (params_.dt / params_.substeps) * 60.0 / (2.0 * kPi);
  const double max_rpm_decel = max_rpm_accel;

  if (target_rpm > state_.rpm) {
    state_.rpm = std::min(state_.rpm + max_rpm_accel, target_rpm);
  } else {
    state_.rpm = std::max(state_.rpm - max_rpm_decel, target_rpm);
  }

  if (state_.rpm >= vehicle_params_.max_rpm * 0.95 && state_.gear < static_cast<int>(vehicle_params_.gear_ratios.size())) {
    state_.gear++;
    target_rpm = compute_rpm(state_.gear);
    state_.rpm = std::max(state_.rpm - max_rpm_accel * 2.0, target_rpm);
  } else if (state_.rpm <= vehicle_params_.idle_rpm * 1.6 && state_.gear > 1) {
    state_.gear--;
    target_rpm = compute_rpm(state_.gear);
    state_.rpm = std::min(state_.rpm + max_rpm_accel * 2.0, target_rpm);
  }
}

// Aerodynamic model with ground effect, wing contribution, and proper front/rear balance.
//
// Computes:
//   Drag: D = 0.5 * rho * Cd * A_drag * v^2 * (1 + wing_drag)
//   Downforce: DF = 0.5 * rho * Cz * A_df * v^2 * ground_effect + wing_downforce
//   Lift: simplified as residual body lift
//
// Ground effect increases downforce at lower ride heights.
// Wings add significant downforce and drag proportional to angle of attack.
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
  const double base_downforce = dynamic_pressure * vehicle_params_.aero_downforce_area * vehicle_params_.downforce_coefficient;

  const double ride_height_factor = 1.0 + vehicle_params_.ground_effect_factor / (vehicle_params_.ride_height + 0.05);
  const double wing_factor = 1.0 + std::sin(vehicle_params_.wing_angle);

  const double wing_drag = dynamic_pressure * (vehicle_params_.front_wing_area + vehicle_params_.rear_wing_area) * std::sin(vehicle_params_.wing_angle) * 0.5;
  const double wing_downforce = dynamic_pressure * (vehicle_params_.front_wing_area + vehicle_params_.rear_wing_area) * std::sin(vehicle_params_.wing_angle);

  const double drag = base_drag * wing_factor + wing_drag;
  const double lift = base_lift * ride_height_factor;
  const double downforce = base_downforce * ride_height_factor + wing_downforce;

  const double total_aero_load = downforce - lift;
  const double wing_balance = (vehicle_params_.front_wing_area + vehicle_params_.rear_wing_area) > kEpsilon
    ? vehicle_params_.rear_wing_area / (vehicle_params_.front_wing_area + vehicle_params_.rear_wing_area)
    : 0.5;

  const Vec2 drag_dir = -state_.velocity.normalized();
  const double drag_decel = drag / vehicle_params_.mass;

  state_.acceleration += drag_dir * drag_decel;
  state_.aero_drag = drag;
  state_.aero_lift = lift;
  state_.aero_downforce = downforce;
  state_.aero_front_balance = 1.0 - wing_balance;
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

  const double target_roll = a_lat * h / (vehicle_params_.front_spring_rate + vehicle_params_.rear_spring_rate);
  const double target_pitch = -a_long * h / (vehicle_params_.front_spring_rate + vehicle_params_.rear_spring_rate);

  const double clamped_roll = std::clamp(target_roll, -vehicle_params_.max_body_roll, vehicle_params_.max_body_roll);
  const double clamped_pitch = std::clamp(target_pitch, -vehicle_params_.max_body_pitch, vehicle_params_.max_body_pitch);

  const double dt = params_.dt / params_.substeps;
  const double k_roll = 1.0 - std::exp(-vehicle_params_.roll_damping * dt * 60.0);
  const double k_pitch = 1.0 - std::exp(-vehicle_params_.pitch_damping * dt * 60.0);

  state_.body_roll += (clamped_roll - state_.body_roll) * k_roll;
  state_.body_pitch += (clamped_pitch - state_.body_pitch) * k_pitch;

  state_.front_camber = vehicle_params_.camber_gain_per_roll * state_.body_roll;
  state_.rear_camber = vehicle_params_.camber_gain_per_roll * state_.body_roll;
}

// Combined slip tire model with friction ellipse.
// Calculates longitudinal and lateral forces for each tire considering
// combined slip conditions (acceleration/braking while cornering).
void Simulation::update_tire_forces(double dt) {
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

  // Kinematic slip angles (bicycle model)
  const double alpha_f_raw = delta - std::atan2(v_y + a_f * omega, v_x);
  const double alpha_r_raw = -std::atan2(v_y - a_r * omega, v_x);

  state_.front_slip_angle = alpha_f_raw;
  state_.rear_slip_angle = alpha_r_raw;
  state_.slip_angle = (std::abs(alpha_f_raw) + std::abs(alpha_r_raw)) * 0.5;

  // Relaxed slip angles (transient behavior)
  state_.front_slip_angle_relaxed = physics::relax_slip_angle(
    alpha_f_raw, state_.front_slip_angle_relaxed, v_x,
    vehicle_params_.tire_relaxation_length, dt);
  state_.rear_slip_angle_relaxed = physics::relax_slip_angle(
    alpha_r_raw, state_.rear_slip_angle_relaxed, v_x,
    vehicle_params_.tire_relaxation_length, dt);

  const double alpha_f = state_.front_slip_angle_relaxed;
  const double alpha_r = state_.rear_slip_angle_relaxed;

  // Grip factors (temperature, wear)
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

  // Track conditions
  const auto& tp = track_->at(state_.distance_along_track);
  const double banking_factor = std::cos(tp.banking);
  const double friction_factor = tp.friction * state_.weather_grip_factor;

  // Tire loads
  double fz_front = state_.fl_tire_load + state_.fr_tire_load;
  double fz_rear = state_.rl_tire_load + state_.rr_tire_load;

  // Calculate longitudinal slip from engine/braking forces
  // Approximate: use total longitudinal acceleration to estimate slip
  const double a_long = state_.acceleration.dot(forward_dir);
  const double engine_force = vehicle_params_.max_torque *
    vehicle_params_.gear_ratios[std::clamp(state_.gear - 1, 0, static_cast<int>(vehicle_params_.gear_ratios.size()) - 1)] *
    vehicle_params_.final_drive * (1.0 - vehicle_params_.drivetrain_loss) / vehicle_params_.wheel_radius;
  const double drive_force = engine_force * state_.throttle;
  const double brake_decel = vehicle_params_.max_brake_force * state_.brake;

  // Longitudinal slip ratio (scaled down for realistic behavior)
  const double net_long_force = drive_force - brake_decel;
  const double sigma_x_rear = clamp(net_long_force / (mu_rear * fz_rear + kEpsilon) * 0.05, -0.3, 0.3);
  const double sigma_x_front = clamp(-brake_decel * 0.3 / (mu_front * fz_front + kEpsilon) * 0.05, -0.3, 0.3);

  state_.slip_ratio = (std::abs(sigma_x_front) + std::abs(sigma_x_rear)) * 0.5;

  // Pacejka parameters
  const double b_long = vehicle_params_.tire_pacejka_b * 0.8;
  const double b_lat = vehicle_params_.tire_pacejka_b;
  const double c = vehicle_params_.tire_pacejka_c;
  const double e = vehicle_params_.tire_pacejka_e;

  // Compute tire forces for each corner
  double fl_fx, fl_fy, fr_fx, fr_fy, rl_fx, rl_fy, rr_fx, rr_fy;

  // Front tires (steered, braking)
  physics::compute_tire_forces(sigma_x_front, alpha_f, state_.fl_tire_load,
    state_.front_camber, vehicle_params_.tire_camber_gain,
    mu_front * friction_factor * banking_factor,
    mu_front * friction_factor * banking_factor,
    b_long, b_lat, c, e, fl_fx, fl_fy);

  physics::compute_tire_forces(sigma_x_front, alpha_f, state_.fr_tire_load,
    state_.front_camber, vehicle_params_.tire_camber_gain,
    mu_front * friction_factor * banking_factor,
    mu_front * friction_factor * banking_factor,
    b_long, b_lat, c, e, fr_fx, fr_fy);

  // Rear tires (driven, braking)
  physics::compute_tire_forces(sigma_x_rear, alpha_r, state_.rl_tire_load,
    state_.rear_camber, vehicle_params_.tire_camber_gain,
    mu_rear * friction_factor * banking_factor,
    mu_rear * friction_factor * banking_factor,
    b_long, b_lat, c, e, rl_fx, rl_fy);

  physics::compute_tire_forces(sigma_x_rear, alpha_r, state_.rr_tire_load,
    state_.rear_camber, vehicle_params_.tire_camber_gain,
    mu_rear * friction_factor * banking_factor,
    mu_rear * friction_factor * banking_factor,
    b_long, b_lat, c, e, rr_fx, rr_fy);

  // Sum forces in vehicle frame
  const double total_fx = fl_fx + fr_fx + rl_fx + rr_fx;
  const double total_fy = fl_fy + fr_fy + rl_fy + rr_fy;

  // Transform to world frame
  const Vec2 longitudinal_force = forward_dir * total_fx;
  const double lateral_force = total_fy;
  state_.acceleration += longitudinal_force / vehicle_params_.mass;
  state_.acceleration += right_dir * (lateral_force / vehicle_params_.mass);

  // Yaw moment from lateral forces and longitudinal differences
  const double front_lateral = fl_fy + fr_fy;
  const double rear_lateral = rl_fy + rr_fy;
  const double yaw_from_lateral = a_f * front_lateral - a_r * rear_lateral;

  // Yaw moment from longitudinal differences (torque vectoring effect)
  const double left_long = fl_fx + rl_fx;
  const double right_long = fr_fx + rr_fx;
  const double yaw_from_long = (right_long - left_long) * vehicle_params_.track_width * 0.5;

  const double total_yaw_moment = yaw_from_lateral + yaw_from_long;
  const double yaw_inertia = vehicle_params_.mass * (a_f * a_f + a_r * a_r) * 0.5;
  state_.yaw_rate = total_yaw_moment / yaw_inertia;
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

// Apply centripetal and centrifugal forces based on track curvature.
// Centripetal force acts toward the center of curvature and is added to the
// vehicle acceleration. Centrifugal force is the inertial reaction and is
// recorded for telemetry / HUD purposes.
void Simulation::update_centripetal_forces() {
  const double speed = state_.speed;
  if (speed < kEpsilon || !track_) {
    state_.centripetal_force = 0.0;
    state_.centrifugal_force = 0.0;
    state_.lateral_g = 0.0;
    return;
  }

  const auto& tp = track_->at(state_.distance_along_track);
  const double kappa = tp.curvature;

  if (std::abs(kappa) < kEpsilon) {
    state_.centripetal_force = 0.0;
    state_.centrifugal_force = 0.0;
    state_.lateral_g = 0.0;
    return;
  }

  const double m = vehicle_params_.mass;
  const Vec2 normal = tp.normal;

  const Vec2 f_c = physics::centripetal_force(m, speed, kappa, normal);
  state_.centripetal_force = f_c.norm();
  state_.centrifugal_force = -state_.centripetal_force;
  state_.lateral_g = state_.centripetal_force / (m * kGravity);

  state_.acceleration += f_c / m;
}

// Velocity-space integration using semi-implicit Euler.
// Heading is updated first, then velocities and position use the new orientation.
// This removes artificial lateral damping and fixes coordinate drift.
void Simulation::integrate(double dt) {
  state_.heading += state_.yaw_rate * dt;
  state_.heading = normalize_angle(state_.heading);

  const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  const Vec2 right_dir(-forward_dir.y(), forward_dir.x());

  const double a_long = state_.acceleration.dot(forward_dir);
  const double a_lat = state_.acceleration.dot(right_dir);

  double new_long_speed = state_.speed + a_long * dt;
  new_long_speed = clamp(new_long_speed, 0.0, 150.0);

  double new_lat_speed = state_.lateral_velocity + a_lat * dt;
  const double lat_speed_max = 30.0;
  new_lat_speed = clamp(new_lat_speed, -lat_speed_max, lat_speed_max);

  state_.velocity = forward_dir * new_long_speed + right_dir * new_lat_speed;
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

void Simulation::apply_box_lane_speed_limit() {
  if (state_.in_box_lane && state_.speed > state_.box_lane_speed) {
    state_.speed = state_.box_lane_speed;
    const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
    state_.velocity = forward_dir * state_.speed;
  }
}

// Off-track physics: simulates driving on grass/gravel outside the tarmac.
//
// When the car is outside the track boundary:
//   1. Grip is reduced proportionally to surface type (grass = 0.30).
//   2. A lateral damping force slows the car down quickly.
//   3. A barrier push-back force is applied when the car has penetrated the
//      virtual guardrail (>1m past track edge), simulating a wall collision.
//
// This prevents the car from drifting infinitely and makes going off-track
// feel like a real penalty without requiring full rigid-body collision.
void Simulation::apply_off_track_physics() {
  if (!track_) return;

  const auto& tp = track_->at(state_.distance_along_track);
  const Vec2 to_car = state_.position - tp.position;
  const double lateral = to_car.dot(tp.normal);
  const double track_half = tp.width / 2.0;

  const bool off_track = std::abs(lateral) > track_half && !state_.in_box_lane;
  if (!off_track) return;

  // --- Grip reduction ---
  // Off-track surface: grass (0.30) or gravel (0.40). Use grass as default.
  const double off_track_grip = 0.30;
  const double grip_scale = off_track_grip / std::max(tp.friction, off_track_grip);

  // Dampen speed quickly (rough terrain rolling resistance)
  const double terrain_drag = 1.0 - 4.0 * (params_.dt / params_.substeps);
  state_.speed = std::max(0.0, state_.speed * terrain_drag);
  const Vec2 forward_dir(std::cos(state_.heading), std::sin(state_.heading));
  state_.velocity = forward_dir * state_.speed;

  // Dampen lateral velocity aggressively
  state_.lateral_velocity *= (1.0 - 8.0 * (params_.dt / params_.substeps));

  // --- Barrier push-back ---
  // Guardrail is modelled at track_half + 1.0 m (runoff zone).
  // Beyond that, apply a spring force proportional to penetration depth.
  const double guardrail_offset = track_half + 1.0;
  const double penetration = std::abs(lateral) - guardrail_offset;

  if (penetration > 0.0) {
    const double barrier_stiffness = 80000.0;  // N/m — stiff wall
    const double barrier_damping  = 5000.0;   // Ns/m — energy absorption
    const double barrier_force_mag = barrier_stiffness * penetration +
                                     barrier_damping * state_.speed;

    // Force direction: push car back toward track center
    const Vec2 push_dir = (lateral > 0.0) ? -tp.normal : tp.normal;
    const Vec2 barrier_force = push_dir * barrier_force_mag;

    // Clamp speed and apply impulse
    state_.velocity += barrier_force * (params_.dt / params_.substeps) / vehicle_params_.mass;
    state_.speed = state_.velocity.norm();
    // Hard cap: car cannot move through the barrier
    if (state_.speed > 0.0) {
      state_.velocity = state_.velocity.normalized() * std::min(state_.speed, 10.0);
      state_.speed = state_.velocity.norm();
    }
  }
}

// Respawn: teleport the car back to the last recorded on-track state.
// The car is placed at the last valid position, heading aligned to the track
// tangent, with zero speed. This is the R-key / collision recovery action.
void Simulation::respawn() {
  if (!track_) return;

  vehicle::VehicleState respawn_state;

  if (has_valid_state_) {
    // Restore last valid on-track position
    respawn_state = last_valid_state_;
  } else {
    // Fall back to track start
    respawn_state.position = track_->get_start_position();
    respawn_state.heading  = track_->get_start_heading();
    respawn_state.distance_along_track = 0.0;
    respawn_state.lap = state_.lap;
  }

  // Zero out all motion
  respawn_state.velocity         = Vec2::Zero();
  respawn_state.acceleration     = Vec2::Zero();
  respawn_state.speed            = 0.0;
  respawn_state.lateral_velocity = 0.0;
  respawn_state.yaw_rate         = 0.0;
  respawn_state.rpm              = vehicle_params_.idle_rpm;
  respawn_state.gear             = 1;

  // Align heading to track tangent at the respawn point
  const auto& tp = track_->at(respawn_state.distance_along_track);
  respawn_state.heading = std::atan2(tp.tangent.y(), tp.tangent.x());

  state_ = respawn_state;
  frames_off_track_ = 0;
}

}
