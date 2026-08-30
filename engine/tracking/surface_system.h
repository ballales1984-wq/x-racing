#pragma once

#include "common.h"
#include "tracking/surface_cell.h"
#include "tracking/surface_grid.h"
#include "tracking/track_position.h"
#include "track/track.h"
#include <memory>

// Project 0 — tracking / dynamic surface system
// Namespace: p0::tracking
namespace p0::tracking {

struct VehicleSurfaceInput {
  TrackPosition track_position;
  double speed = 0.0;
  double load = 1.0;        // normalized load factor (1.0 = nominal)
  bool on_track = false;
};

// Updates surface temperature, rubber deposition, moisture and grip
// based on vehicle passes and ambient conditions.
class SurfaceSystem {
 public:
  explicit SurfaceSystem(const p0::track::Track& track);
  ~SurfaceSystem() = default;

  void set_ambient_temperature(double temp);
  void set_ambient_moisture(double moisture);

  void update(double dt);
  void apply_vehicle(const VehicleSurfaceInput& input);

  const SurfaceGrid& grid() const { return grid_; }
  double grip_at(const TrackPosition& pos) const;

 private:
  void update_cell_physics(SurfaceCell& cell, double dt);
  void deposit_rubber(SurfaceCell& cell, double speed, double load);
  double grip_model(const SurfaceCell& cell) const;

  SurfaceGrid grid_;
  double ambient_temperature_ = 20.0;
  double ambient_moisture_ = 0.0;
};

}  // namespace p0::tracking
