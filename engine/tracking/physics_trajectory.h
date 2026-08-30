#pragma once

#include "common.h"
#include "tracking/position_provider.h"
#include "tracking/coordinate_converter.h"
#include "vehicle/vehicle.h"

// Project 0 — tracking / physics trajectory source
// Namespace: p0::tracking
namespace p0::tracking {

// ITrajectorySource that reads from the live physics VehicleState.
// Converts the simulation's local position into a geographic PositionSample
// via a CoordinateConverter.
//
// Usage:
//   PhysicsTrajectory traj(converter, &vehicle_state);
//   SimulatedGPS gps(std::make_unique<PhysicsTrajectory>(traj));
class PhysicsTrajectory : public ITrajectorySource {
 public:
  PhysicsTrajectory(CoordinateConverter converter,
                    const p0::vehicle::VehicleState* state);

  void set_state(const p0::vehicle::VehicleState* state) { state_ = state; }

  PositionSample sample_at(double time) override;
  double duration() const override { return kInfiniteDuration; }

 private:
  CoordinateConverter converter_;
  const p0::vehicle::VehicleState* state_ = nullptr;
  static constexpr double kInfiniteDuration = 1e9;
};

}  // namespace p0::tracking
