#pragma once

#include "common.h"

namespace p0::vehicle {

struct VehicleParams {
  double mass = 1200.0;
  double wheelbase = 2.5;
  double track_width = 1.5;
  double cg_height = 0.4;
  double cg_to_front = 1.25;
  double cg_to_rear = 1.25;
  double wheel_radius = 0.3;
  double max_steer_angle = 35.0 * kDegToRad;
  double steer_ratio = 16.0;
  double max_power = 250000.0;
  double max_torque = 300.0;
  double idle_rpm = 800.0;
  double max_rpm = 7500.0;
  double final_drive = 3.5;
  std::vector<double> gear_ratios = {3.5, 2.3, 1.6, 1.2, 0.9, 0.7};
  double drag_coefficient = 0.35;
  double frontal_area = 2.0;
  double lift_coefficient = 0.1;
  double rolling_resistance = 0.015;
  double max_brake_force = 18000.0;
  double tire_mu = 1.0;
  double tire_pacejka_b = 10.0;
  double tire_pacejka_c = 1.9;
  double tire_pacejka_e = 0.97;
  double suspension_stiffness = 30000.0;
  double suspension_damping = 2500.0;
  double anti_roll_bar = 15000.0;
};

struct VehicleState {
  Vec2 position{0.0, 0.0};
  Vec2 velocity{0.0, 0.0};
  Vec2 acceleration{0.0, 0.0};
  double heading = 0.0;
  double yaw_rate = 0.0;
  double steer_angle = 0.0;
  double throttle = 0.0;
  double brake = 0.0;
  double rpm = 800.0;
  int gear = 1;
  double slip_ratio = 0.0;
  double slip_angle = 0.0;
  double speed = 0.0;
  double distance_along_track = 0.0;
  int lap = 0;
};

}
