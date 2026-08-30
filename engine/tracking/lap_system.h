#pragma once

#include "common.h"
#include "tracking/track_position.h"
#include "track/lap_detector.h"

// Project 0 — tracking / lap system
// Namespace: p0::tracking
namespace p0::tracking {

// Lap timing event produced by LapSystem.
struct LapEvent {
  enum class Type {
    kLapStarted,
    kLapFinished,
    kLapInvalidated,
    kSectorPassed
  };

  Type type = Type::kLapStarted;
  double timestamp = 0.0;
  double lap_time = 0.0;
  int lap_number = 0;
  int sector = 0;
};

// State machine for lap detection driven by track-relative position.
// Decouples lap logic from raw distance inputs by consuming TrackPosition.
class LapSystem {
 public:
  LapSystem() = default;
  explicit LapSystem(double track_length, int total_laps = 0);
  LapSystem(const LapSystem&) = default;
  LapSystem(LapSystem&&) = default;
  LapSystem& operator=(const LapSystem&) = default;
  LapSystem& operator=(LapSystem&&) = default;
  ~LapSystem() = default;

  void update(const TrackPosition& pos, double timestamp);
  void reset();

  bool lap_started() const { return lap_started_; }
  bool lap_finished() const { return lap_finished_; }
  bool lap_invalidated() const { return lap_invalidated_; }
  int completed_laps() const { return lap_detector_.completed_laps(); }
  int total_laps() const { return lap_detector_.total_laps(); }
  bool finished() const { return lap_detector_.finished(); }
  double current_lap_time() const { return current_lap_time_; }
  double last_lap_time() const { return last_lap_time_; }

  std::vector<LapEvent> pop_events();

 private:
  void emit_event(LapEvent::Type type, double timestamp);

  p0::track::LapDetector lap_detector_;
  bool lap_started_ = false;
  bool lap_finished_ = false;
  bool lap_invalidated_ = false;
  double lap_start_time_ = 0.0;
  double current_lap_time_ = 0.0;
  double last_lap_time_ = 0.0;
  int lap_number_ = 0;
  std::vector<LapEvent> events_;
};

}  // namespace p0::tracking
