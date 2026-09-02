#pragma once

#include "common.h"
#include "tracking/position_provider.h"
#include "tracking/track_position.h"
#include "tracking/lap_system.h"
#include "tracking/surface_system.h"
#include "track/track.h"
#include <memory>

// Project 0 — tracking / surface-aware telemetry
// Namespace: p0::tracking
namespace p0::tracking {

// Telemetry frame enriched with track-relative and surface data.
struct TrackedFrame {
  double timestamp = 0.0;
  int lap_number = 0;

  double s = 0.0;
  double lateral = 0.0;
  double speed = 0.0;
  double heading = 0.0;

  // Geographic position reported by the position provider (GPS/replay).
  double latitude = 0.0;
  double longitude = 0.0;
  double altitude = 0.0;
  double gps_speed = 0.0;
  double gps_heading = 0.0;
  double gps_accuracy = 0.0;

  double grip = 1.0;
  double surface_temp = 20.0;
  double rubber = 0.0;

  double lap_time = 0.0;
};

// Records tracked frames during a session and exports them to CSV.
class TrackedTelemetry {
 public:
  explicit TrackedTelemetry(const std::string& output_path = "tracked_telemetry.csv");

  void record(const TrackPosition& pos,
              const PositionSample& gps,
              double grip,
              double surface_temp,
              double rubber,
              const LapSystem& lap_system);

  void save_csv() const;
  void clear();

  const std::vector<TrackedFrame>& frames() const { return frames_; }

 private:
  std::vector<TrackedFrame> frames_;
  std::string output_path_;
  int current_lap_ = 0;
};

}  // namespace p0::tracking
