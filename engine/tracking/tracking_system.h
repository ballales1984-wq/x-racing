#pragma once

#include <memory>
#include "common.h"
#include "tracking/position_provider.h"
#include "tracking/track_position.h"
#include "tracking/track_mapper.h"
#include "tracking/clock.h"

// Project 0 — tracking / central tracking system
// Namespace: p0::tracking
namespace p0::tracking {

// Central tracking system that consumes any IPositionProvider
// and exposes the current track-relative state to downstream systems
// (Lap, Surface, Telemetry).
class TrackingSystem {
 public:
  explicit TrackingSystem(
      std::unique_ptr<IPositionProvider> provider,
      const IClock* clock = nullptr);

  ~TrackingSystem() = default;

  bool start();
  void stop();
  void update();

  const PositionSample& current_sample() const { return current_sample_; }
  const TrackPosition& current_track_position() const { return current_track_; }
  bool is_running() const { return running_; }

  IPositionProvider* provider() const { return provider_.get(); }

  void set_mapper(std::unique_ptr<TrackMapper> mapper);

 private:
  std::unique_ptr<IPositionProvider> provider_;
  std::unique_ptr<TrackMapper> mapper_;
  const IClock* clock_;

  PositionSample current_sample_;
  TrackPosition current_track_;
  bool running_ = false;
};

}  // namespace p0::tracking
