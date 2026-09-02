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
  int lap_number = 0;                    // current lap number
  double lap_time = 0.0;                 // s, time within the current lap
  double last_lap_time = 0.0;            // s, duration of the most recently completed lap
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
  double front_tire_temp = 0.0;           // K, front tire temperature
  double rear_tire_temp = 0.0;            // K, rear tire temperature
  double front_tire_wear = 0.0;           // [0,1], front tire wear
  double rear_tire_wear = 0.0;            // [0,1], rear tire wear
};

// Telemetry recorder: accumulates frames and exports to CSV.
// Designed for offline analysis with Python/Matplotlib.
class Telemetry {
 public:
  void record(const vehicle::VehicleState& state, double dt);
  void mark_lap(int lap_number) { pending_lap_number_ = lap_number; }
  const std::vector<TelemetryFrame>& frames() const { return frames_; }
  void clear() {
    frames_.clear();
    current_lap_ = 0;
    current_lap_time_ = 0.0;
    last_lap_time_ = 0.0;
    pending_lap_number_ = 0;
  }
  void save_csv(const std::string& path) const;

 private:
  std::vector<TelemetryFrame> frames_;
  int current_lap_ = 0;
  double current_lap_time_ = 0.0;
  double last_lap_time_ = 0.0;
  // Lap number announced by the caller via mark_lap() but not yet committed
  // to the current_lap_ counter; the next record() call promotes it.
  int pending_lap_number_ = 0;
};

}
