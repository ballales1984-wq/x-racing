#include "telemetry/telemetry.h"
#include <fstream>
#include <iomanip>
#include <sstream>

namespace p0::telemetry {

void Telemetry::record(const vehicle::VehicleState& state, double dt) {
  TelemetryFrame frame;
  frame.time += dt;
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

  if (frames_.empty()) {
    frame.time = 0.0;
  } else {
    frame.time = frames_.back().time + dt;
  }

  const double speed = state.speed;
  if (speed > kEpsilon) {
    const Vec2 lateral_axis(-std::sin(state.heading), std::cos(state.heading));
    const double lat_accel = state.acceleration.dot(lateral_axis) / kGravity;
    const double long_accel = state.acceleration.dot(state.velocity.normalized()) / kGravity;
    frame.lateral_g = lat_accel;
    frame.longitudinal_g = long_accel;
  }

  frames_.push_back(frame);
}

void Telemetry::save_csv(const std::string& path) const {
  std::ofstream file(path);
  if (!file.is_open()) return;

  file << "time,distance,speed,rpm,gear,throttle,brake,steer,slip_angle,slip_ratio,"
       << "pos_x,pos_y,vel_x,vel_y,acc_x,acc_y,heading,lateral_g,longitudinal_g\n";

  for (const auto& frame : frames_) {
    file << std::fixed << std::setprecision(6)
         << frame.time << ","
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
         << frame.longitudinal_g << "\n";
  }
}

}
