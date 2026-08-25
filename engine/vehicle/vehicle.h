#pragma once

#include "common.h"

// Project 0 — vehicle definition
// Namespace: p0::vehicle
namespace p0::vehicle {

// Vehicle parameters: constant physical properties of the car
// These values define the baseline behavior; kit/setup modifications
// will change a subset of these fields.
struct VehicleParams {
  double mass = 1500.0;                  // kg, Porsche 911 ~1500kg
  double wheelbase = 2.45;               // m, typical sports car
  double track_width = 1.6;              // m, wider for stability
  double cg_height = 0.35;               // m, low center of gravity
  double cg_to_front = 1.4;              // m, rear-engine bias
  double cg_to_rear = 1.05;              // m
  double wheel_radius = 0.33;            // m, 20" wheels
  double max_steer_angle = 30.0 * kDegToRad; // rad, sporty but not race
  double steer_ratio = 15.0;             // quick steering
  double max_power = 350000.0;           // W, 470 hp (911 Turbo S)
  double max_torque = 530.0;             // Nm
  double idle_rpm = 800.0;               // rpm
  double max_rpm = 7200.0;               // rpm
  double final_drive = 3.44;             // final drive ratio
  double engine_inertia = 0.25;          // kg*m^2
  double drivetrain_loss = 0.12;         // fraction
  std::vector<double> gear_ratios = {3.67, 2.0, 1.35, 1.0, 0.85, 0.7};
  double drag_coefficient = 0.33;        // Cd, sleek sports car
  double frontal_area = 2.1;             // m^2
  double lift_coefficient = -0.1;        // slight body downforce
  double downforce_coefficient = 0.3;    // rear wing contribution
  double aero_downforce_area = 1.8;      // m^2
  double ground_effect_factor = 0.03;    // ground effect
  double front_wing_area = 0.1;          // m^2, minimal
  double rear_wing_area = 0.3;           // m^2, rear spoiler
  double wing_angle = 0.1;               // rad, modest angle
  double rolling_resistance = 0.012;     // performance tires
  double max_brake_force = 20000.0;      // N, strong brakes
  double tire_mu = 1.2;                  // performance road tires
  double tire_pacejka_b = 11.0;          // Pacejka stiffness
  double tire_pacejka_c = 1.9;           // Pacejka shape
  double tire_pacejka_e = 0.97;          // Pacejka curvature
  double tire_relaxation_length = 0.4;   // m, quicker response
  double tire_camber_gain = 1.0;         // camber gain
  double tire_load_sensitivity = 0.06;   // load sensitivity
  double tire_max_reference_load = 5000.0; // N
  double front_spring_rate = 35000.0;    // N/m, sport suspension
  double rear_spring_rate = 38000.0;     // N/m, stiffer rear
  double front_damping = 3500.0;         // Ns/m
  double rear_damping = 3800.0;          // Ns/m
  double ride_height = 0.12;             // m, low ride height
  double anti_roll_bar_stiffness = 12000.0; // Nm/rad
  double max_body_roll = 0.08;           // rad, limited roll
  double max_body_pitch = 0.06;          // rad
  double roll_damping = 0.85;            // damping
  double pitch_damping = 0.85;           // damping
  double camber_gain_per_roll = -3.0;    // good camber compensation
  double ambient_temperature = 295.0;    // K, 22°C
  double tire_optimal_temp = 340.0;      // K, 67°C
  double tire_temp_curve_width = 25.0;   // K, wider operating window
  double tire_cooling_rate = 0.4;        // K/s
  double tire_heat_per_slip = 1.5;       // K per slip
  double tire_wear_per_slip = 0.00008;   // wear
  double tire_wear_per_lap = 0.0008;     // base wear
  double rain_intensity = 0.0;           // [0, 1]
  double rain_grip_reduction = 0.35;     // grip reduction
  double rain_rolling_resistance = 1.2;  // rolling resistance multiplier
  double wind_effect_on_speed = 0.08;    // wind effect
  double temp_cooling_rate = 0.15;       // K/s
  double rain_cooling = 1.5;             // K/s
  double track_heat_rate = 0.04;         // K/s
};

// Vehicle state: instantaneous values updated each simulation step
struct VehicleState {
  Vec2 position{0.0, 0.0};              // m, world position
  Vec2 velocity{0.0, 0.0};              // m/s, world velocity
  Vec2 acceleration{0.0, 0.0};          // m/s^2, world acceleration
  double heading = 0.0;                  // rad, yaw angle
  double yaw_rate = 0.0;                 // rad/s, rate of change of heading
  double steer_angle = 0.0;              // rad, current steering angle
  double throttle = 0.0;                 // [0,1], accelerator pedal
  double brake = 0.0;                    // [0,1], brake pedal
  double rpm = 800.0;                    // engine RPM
  int gear = 1;                          // current gear (1-based)
  double slip_ratio = 0.0;               // longitudinal slip [-1, +1]
  double slip_angle = 0.0;               // lateral slip angle, rad
  double front_slip_angle = 0.0;         // front axle slip angle, rad
  double rear_slip_angle = 0.0;          // rear axle slip angle, rad
  double front_slip_angle_relaxed = 0.0; // rad, filtered front slip angle
  double rear_slip_angle_relaxed = 0.0;  // rad, filtered rear slip angle
  double front_camber = 0.0;             // rad, front wheel camber angle
  double rear_camber = 0.0;              // rad, rear wheel camber angle
  double lateral_velocity = 0.0;         // m/s, velocity perpendicular to heading
  double speed = 0.0;                    // m/s, scalar speed
  double aero_lift = 0.0;                // N, aerodynamic lift force
  double aero_drag = 0.0;                // N, aerodynamic drag force
  double aero_downforce = 0.0;           // N, aerodynamic downforce
  double aero_front_balance = 0.5;       // [0,1], front aero distribution
  double front_tire_temp = 300.0;        // K, front tire temperature
  double rear_tire_temp = 300.0;         // K, rear tire temperature
  double front_tire_wear = 1.0;          // [0,1], front tire wear (1=new)
  double rear_tire_wear = 1.0;           // [0,1], rear tire wear (1=new)
  double distance_along_track = 0.0;     // m, progress along track centerline
  int lap = 0;                           // completed laps
  double fl_tire_load = 0.0;             // N, front-left tire normal force
  double fr_tire_load = 0.0;             // N, front-right tire normal force
  double rl_tire_load = 0.0;             // N, rear-left tire normal force
  double rr_tire_load = 0.0;             // N, rear-right tire normal force
  double body_roll = 0.0;                // rad, body roll angle
  double body_pitch = 0.0;               // rad, body pitch angle
  bool in_box_lane = false;              // true if vehicle is currently in the box/pit lane
  bool box_lane_entry_requested = false; // true if vehicle requested box lane entry
  double box_lane_speed = 0.0;           // m/s, speed limit in box lane
  double weather_grip_factor = 1.0;       // [-], weather-based grip reduction
  double track_temp = 305.0;             // K, local track temperature
  double centripetal_force = 0.0;        // N, toward center of curvature
  double centrifugal_force = 0.0;        // N, outward inertial force
  double lateral_g = 0.0;                // g, lateral acceleration / gravity
};

}
