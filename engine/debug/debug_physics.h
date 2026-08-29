#pragma once

#include "common.h"
#include "vehicle/vehicle.h"
#include <string>

namespace p0::debug {

struct PhysicsDiagnostics {
  bool on_track = true;
  bool in_box_lane = false;
  double track_half_width = 6.0;
  double lateral_position = 0.0;
  double distance_to_barrier = 0.0;
  bool tire_temp_critical = false;
  bool tire_wear_critical = false;
  bool spin_detected = false;
  bool collision_detected = false;
  bool excessive_slip = false;
  double peak_slip_angle = 0.0;
  double peak_slip_ratio = 0.0;
  double max_tire_load_imbalance = 0.0;
  std::string tire_status;
  std::string suspension_status;
  std::string aero_status;
};

class DebugPhysics {
 public:
  DebugPhysics() = default;

  void initialize();
  void shutdown();

  void analyze(const p0::vehicle::VehicleState& state, double dt);
  const PhysicsDiagnostics& current_diagnostics() const { return diagnostics_; }
  void reset();

 private:
  void analyze_tire_temps(const p0::vehicle::VehicleState& state);
  void analyze_tire_wear(const p0::vehicle::VehicleState& state);
  void analyze_suspension(const p0::vehicle::VehicleState& state);
  void analyze_aero(const p0::vehicle::VehicleState& state);
  void analyze_slip(const p0::vehicle::VehicleState& state);
  std::string classify_tire_status(const p0::vehicle::VehicleState& state) const;
  std::string classify_suspension_status(const p0::vehicle::VehicleState& state) const;
  std::string classify_aero_status(const p0::vehicle::VehicleState& state) const;

  PhysicsDiagnostics diagnostics_;
  double peak_slip_angle_frame_ = 0.0;
  double peak_slip_ratio_frame_ = 0.0;
  double max_load_imbalance_ = 0.0;
  int critical_tire_frames_ = 0;
  bool was_on_track_ = true;
  double off_track_accum_ = 0.0;
};

}
