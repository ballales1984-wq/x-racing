#include "tracking/physics_trajectory.h"

namespace p0::tracking {

PhysicsTrajectory::PhysicsTrajectory(CoordinateConverter converter,
                                     const p0::vehicle::VehicleState* state)
    : converter_(std::move(converter)), state_(state) {}

PositionSample PhysicsTrajectory::sample_at(double time) {
  PositionSample sample{};
  if (!state_) return sample;

  const double speed = state_->velocity.norm();
  const double heading = state_->heading;

  sample = converter_.local_to_geographic(
      state_->position, heading, speed, time);
  sample.valid = true;
  return sample;
}

}  // namespace p0::tracking
