#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include "track/track.h"

// Project 0 — telemetry recording and export
// Namespace: p0::telemetry
namespace p0::telemetry {

// One frame of telemetry data recorded per simulation step
struct TelemetryFrame {
  double time = 0.0;                     // s, cumulative time
  double distance = 0.0;                 // m, distance along track
  double speed = 0.0;                     // m/s
  double rpm = 0.0;                       // engine RPM
  int gear = 0;                           // current gear
  double throttle = 0.0;                  // [0,1]
  double brake = 0.0;                     // [0,1]
  double steer = 0.0;                     // rad, steering angle
  double slip_angle = 0.0;                // rad, lateral slip
  double slip_ratio = 0.0;                // [-], longitudinal slip
  Vec2 position{0.0, 0.0};              // m, world position
  Vec2 velocity{0.0, 0.0};              // m/s
  Vec2 acceleration{0.0, 0.0};          // m/s^2
  double heading = 0.0;                   // rad, yaw angle
  double lateral_g = 0.0;                 // g, lateral acceleration
  double longitudinal_g = 0.0;            // g, longitudinal acceleration
};

// Telemetry recorder: accumulates frames and exports to CSV.
// Designed for offline analysis with Python/Matplotlib.
class Telemetry {
 public:
  void record(const vehicle::VehicleState& state, double dt);
  const std::vector<TelemetryFrame>& frames() const { return frames_; }
  void clear() { frames_.clear(); }
  void save_csv(const std::string& path) const;

 private:
  std::vector<TelemetryFrame> frames_;
};

}
