#include "telemetry/telemetry.h"
#include <fstream>
#include <iomanip>
#include <sstream>

// Project 0 — telemetry recording and CSV export
// Designed for offline analysis with Python, NumPy, and Matplotlib.
namespace p0::telemetry {

// Record one frame of telemetry data from the current vehicle state.
// dt is the timestep since the last frame (used for cumulative time).
void Telemetry::record(const vehicle::VehicleState& state, double dt) {
  TelemetryFrame frame;
  frame.lap_number = state.lap;
  frame.lap_time = current_lap_time_;
  frame.last_lap_time = last_lap_time_;
  frame.distance = state.distance_along_track;
  frame.speed = state.speed;
  frame.rpm = state.rpm;
  frame.gear = state.gear;
  frame.throttle = state.throttle;
  frame.brake = state.brake;
  frame.steer = state.steer_angle;
  frame.slip_angle = state.slip_angle;
  frame.slip_ratio = state.slip_ratio;
  frame.position = state.position;
  frame.velocity = state.velocity;
  frame.acceleration = state.acceleration;
  frame.heading = state.heading;
  frame.front_tire_temp = state.front_tire_temp;
  frame.rear_tire_temp = state.rear_tire_temp;
   frame.front_tire_wear = state.front_tire_wear;
   frame.rear_tire_wear = state.rear_tire_wear;

  if (frames_.empty()) {
    frame.time = 0.0;
  } else {
    frame.time = frames_.back().time + dt;
  }

  // Compute lateral and longitudinal acceleration in g-forces
  const double speed = state.speed;
  if (speed > kEpsilon) {
    const Vec2 lateral_axis(-std::sin(state.heading), std::cos(state.heading));
    const double lat_accel = state.acceleration.dot(lateral_axis) / kGravity;
    const double long_accel = state.acceleration.dot(state.velocity.normalized()) / kGravity;
    frame.lateral_g = lat_accel;
    frame.longitudinal_g = long_accel;
  }

  current_lap_time_ += dt;

  // Determine the lap number to record. Prefer an explicit mark_lap()
  // announcement (the caller detected the transition this frame); otherwise
  // fall back to the state-reported lap. Capture the just-completed lap
  // duration on the first frame of the new lap.
  int effective_lap = state.lap;
  if (pending_lap_number_ > current_lap_) {
    effective_lap = pending_lap_number_;
  }
  if (effective_lap > current_lap_) {
    last_lap_time_ = current_lap_time_;
    current_lap_time_ = 0.0;
    current_lap_ = effective_lap;
  }
  pending_lap_number_ = 0;
  frame.lap_number = current_lap_;
  frame.lap_time = current_lap_time_;
  frame.last_lap_time = last_lap_time_;

  frames_.push_back(frame);
}

// Export recorded frames to a CSV file for external analysis.
// Columns: time, lap_number, lap_time, last_lap_time, distance, speed, rpm, gear,
//          throttle, brake, steer, slip_angle, slip_ratio, pos_x, pos_y, vel_x, vel_y,
//          acc_x, acc_y, heading, lateral_g, longitudinal_g,
//          front_tire_temp, rear_tire_temp, front_tire_wear, rear_tire_wear
void Telemetry::save_csv(const std::string& path) const {
  std::ofstream file(path);
  if (!file.is_open()) return;

  file << "time,lap_number,lap_time,last_lap_time,distance,speed,rpm,gear,throttle,brake,steer,slip_angle,slip_ratio,"
       << "pos_x,pos_y,vel_x,vel_y,acc_x,acc_y,heading,lateral_g,longitudinal_g,"
       << "front_tire_temp,rear_tire_temp,front_tire_wear,rear_tire_wear\n";

  for (const auto& frame : frames_) {
    file << std::fixed << std::setprecision(6)
         << frame.time << ","
         << frame.lap_number << ","
         << frame.lap_time << ","
         << frame.last_lap_time << ","
         << frame.distance << ","
         << frame.speed << ","
         << frame.rpm << ","
         << frame.gear << ","
         << frame.throttle << ","
         << frame.brake << ","
         << frame.steer << ","
         << frame.slip_angle << ","
         << frame.slip_ratio << ","
         << frame.position.x() << ","
         << frame.position.y() << ","
         << frame.velocity.x() << ","
         << frame.velocity.y() << ","
         << frame.acceleration.x() << ","
         << frame.acceleration.y() << ","
         << frame.heading << ","
         << frame.lateral_g << ","
         << frame.longitudinal_g << ","
         << frame.front_tire_temp << ","
         << frame.rear_tire_temp << ","
         << frame.front_tire_wear << ","
         << frame.rear_tire_wear << "\n";
  }
}

}
