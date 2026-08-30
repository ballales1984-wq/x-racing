#include "tracking/surface_system.h"

#include <algorithm>
#include <cmath>

namespace p0::tracking {

SurfaceSystem::SurfaceSystem(const p0::track::Track& track)
    : grid_(track.length(), 14.0, 1.0, 0.5) {
  for (SurfaceCell& cell : grid_) {
    cell.base_grip = 1.0;
    cell.grip = 1.0;
  }
}

void SurfaceSystem::set_ambient_temperature(double temp) { ambient_temperature_ = temp; }

void SurfaceSystem::set_ambient_moisture(double moisture) { ambient_moisture_ = moisture; }

void SurfaceSystem::update(double dt) {
  for (SurfaceCell& cell : grid_) {
    update_cell_physics(cell, dt);
    cell.grip = grip_model(cell);
  }
}

void SurfaceSystem::apply_vehicle(const VehicleSurfaceInput& input) {
  if (!input.on_track) return;

  SurfaceCell& cell = grid_.cell(input.track_position.s, input.track_position.lateral);
  deposit_rubber(cell, input.speed, input.load);
  cell.temperature += std::abs(input.speed) * 0.005 * input.load;
}

void SurfaceSystem::update_cell_physics(SurfaceCell& cell, double dt) {
  const double diff = ambient_temperature_ - cell.temperature;
  cell.temperature += diff * dt * 0.1;

  cell.rubber *= std::pow(0.95, dt);
  if (cell.rubber < 0.0) cell.rubber = 0.0;

  cell.moisture += (ambient_moisture_ - cell.moisture) * dt * 0.2;
  if (cell.moisture < 0.0) cell.moisture = 0.0;
}

void SurfaceSystem::deposit_rubber(SurfaceCell& cell, double speed, double load) {
  const double deposit = (speed / 80.0) * load * 0.01;
  cell.rubber = std::min(cell.rubber + deposit, 2.0);
}

double SurfaceSystem::grip_model(const SurfaceCell& cell) const {
  const double rubber_effect = std::min(cell.rubber * 0.15, 0.25);
  const double temp_effect = -std::abs(cell.temperature - 85.0) * 0.001;
  const double water_effect = cell.moisture * 0.4;
  double grip = cell.base_grip + rubber_effect + temp_effect - water_effect;
  return clamp(grip, 0.2, 1.5);
}

double SurfaceSystem::grip_at(const TrackPosition& pos) const {
  return grid_.cell(pos.s, pos.lateral).grip;
}

}  // namespace p0::tracking
