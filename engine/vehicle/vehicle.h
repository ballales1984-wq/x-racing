#pragma once

#include "common.h"

// Project 0 — vehicle definition
// Namespace: p0::vehicle
namespace p0::vehicle {

// Vehicle parameters: constant physical properties of the car
// These values define the baseline behavior; kit/setup modifications
// will change a subset of these fields.
struct VehicleParams {
  double mass = 1200.0;                  // kg
  double wheelbase = 2.5;                // m, distance between axles
  double track_width = 1.5;              // m, distance between left/right tires
  double cg_height = 0.4;                // m, center of mass height from ground
  double cg_to_front = 1.25;             // m, CG distance to front axle
  double cg_to_rear = 1.25;              // m, CG distance to rear axle
  double wheel_radius = 0.3;             // m
  double max_steer_angle = 35.0 * kDegToRad; // rad, maximum wheel angle
  double steer_ratio = 16.0;             // steering wheel turns per lock
  double max_power = 250000.0;           // W
  double max_torque = 300.0;             // Nm
  double idle_rpm = 800.0;               // rpm
  double max_rpm = 7500.0;               // rpm
  double final_drive = 3.5;              // final drive ratio
  std::vector<double> gear_ratios = {3.5, 2.3, 1.6, 1.2, 0.9, 0.7};
  double drag_coefficient = 0.35;        // aerodynamic drag coefficient Cd
  double frontal_area = 2.0;             // m^2, frontal cross-section
  double lift_coefficient = 0.1;         // aerodynamic lift coefficient Cl
  double downforce_coefficient = 0.5;    // aerodynamic downforce coefficient Cz
  double front_wing_area = 0.5;          // m^2, front wing area
  double rear_wing_area = 0.5;           // m^2, rear wing area
  double ride_height_sensitivity = 2.0;  // downforce per meter of ride height
  double pitch_sensitivity = 0.5;        // front/rear balance change per rad of pitch
  double roll_sensitivity = 0.3;         // downforce change per rad of roll
  double wing_angle = 0.0;               // rad, wing angle of attack
  double rolling_resistance = 0.015;     // rolling resistance coefficient
  double max_brake_force = 18000.0;      // N, maximum braking force
  double tire_mu = 1.0;                  // tire-road friction coefficient
  double tire_pacejka_b = 10.0;          // Pacejka stiffness factor
  double tire_pacejka_c = 1.9;           // Pacejka shape factor
  double tire_pacejka_e = 0.97;          // Pacejka curvature factor
  double suspension_stiffness = 30000.0; // N/m (reserved for future use)
  double suspension_damping = 2500.0;    // Ns/m (reserved for future use)
  double anti_roll_bar = 15000.0;        // Nm/deg (reserved for future use)
  double front_spring_rate = 30000.0;    // N/m, front suspension stiffness
  double rear_spring_rate = 30000.0;     // N/m, rear suspension stiffness
  double front_damping = 2500.0;         // Ns/m, front suspension damping
  double rear_damping = 2500.0;          // Ns/m, rear suspension damping
  double ride_height = 0.15;             // m, static ride height
  double anti_roll_bar_stiffness = 15000.0; // Nm/rad, anti-roll bar
  double ambient_temperature = 300.0;    // K, track/air temperature
  double tire_optimal_temp = 350.0;      // K, optimal operating temperature
  double tire_temp_curve_width = 20.0;   // K, width of the grip-vs-temp bell curve
  double tire_cooling_rate = 0.5;        // K/s per K of temperature difference
  double tire_heat_per_slip = 2.0;       // K per unit of slip angle
  double tire_wear_per_slip = 0.0001;    // wear per unit of slip angle
  double tire_wear_per_lap = 0.001;      // base wear per lap
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
};

}
