#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include "track/track.h"

namespace p0::telemetry {

struct TelemetryFrame {
  double time = 0.0;
  double distance = 0.0;
  double speed = 0.0;
  double rpm = 0.0;
  int gear = 0;
  double throttle = 0.0;
  double brake = 0.0;
  double steer = 0.0;
  double slip_angle = 0.0;
  double slip_ratio = 0.0;
  Vec2 position{0.0, 0.0};
  Vec2 velocity{0.0, 0.0};
  Vec2 acceleration{0.0, 0.0};
  double heading = 0.0;
  double lateral_g = 0.0;
  double longitudinal_g = 0.0;
};

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
